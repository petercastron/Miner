#include "loggerLocal.h"
#include "auxHelper.h"
#include "local_defs.h"
#include "hireManager.h"
#include "hireversion.h"
#include <unistd.h>
#include <getopt.h>
#include <signal.h>

#define PRODUCT_NAME  "hirefs"
#define TEMP_DIR      "/tmp"
#define LOG_DIR       TEMP_DIR "/" PRODUCT_NAME

static bool bMainExit = false;

void daemonShutdown();
void signal_handler(int);

void daemonShutdown()
{
  debugEntry(LL_INFO, LOG_MODULE_INDEX_HIRE, "\nreceived exit signal.\n");
  bMainExit = true;
}

void signal_handler(int sig)
{
  switch (sig)
  {
  case SIGHUP:
    /* XXX: handle kill -HUP somehow */
    debugEntry(LL_INFO, LOG_MODULE_INDEX_HIRE, "Received SIGHUP signal.");
    break;
  case SIGINT:
  case SIGTERM:
  case SIGUSR1:
    debugEntry(LL_INFO, LOG_MODULE_INDEX_HIRE, "Daemon exiting");
    daemonShutdown();
    //exit(EXIT_SUCCESS);
    break;
  default:
    debugEntry(LL_INFO, LOG_MODULE_INDEX_HIRE, "Unhandled signal %s", strsignal(sig));
    break;
  }
}

bool check_instance(const char *pidfile)
{
  int pidFilehandle;
  //Ensure only one copy
  {
    pidFilehandle = open(pidfile, O_RDWR | O_CREAT, 0600);
    if (-1 == pidFilehandle) {
      printf("Could not open PID file %s\n", pidfile);
      return false;
    }

    char pid_buf[64] = {0};
    int old_pid = -1;
    if (read(pidFilehandle, pid_buf, sizeof(pid_buf)) > 0) {
      sscanf(pid_buf, "%d", &old_pid);
      if (0 == kill(old_pid, 0)) { //Whether the process exists
        printf("You cannot run multiple %s instances : process exists pid %d \n", PRODUCT_NAME, old_pid);
        ::close(pidFilehandle);
        pidFilehandle = -1;
        return false;
      }
      else {
        ftruncate(pidFilehandle, 0);
        lseek(pidFilehandle, 0, SEEK_SET);
      }
    }
  }
  //write pid
  {
    if (-1 != pidFilehandle) {
      /* Get and format PID */
      char str[10];
      snprintf(str, sizeof str, "%d\n", getpid());

      /* write pid to lockfile */
      if (write(pidFilehandle, str, strlen(str)) < strlen(str)) {
        printf("Failed to write process id into PID file.\n");
        return false;
      };
      ::close(pidFilehandle);
    }
  }
  return true;
}

bool daemonize(bool run_as_daemon)
{
  int pid, sid;

  if (true == run_as_daemon) {
    /*First Fork*/
    pid = fork();
    if (pid < 0) {
      /* Could not fork */
      printf("Could not fork() child process\n");
      return false;
    }

    if (pid > 0) {
      /*First child created ok, so wait it */
      if (pid == waitpid(pid, NULL, 0))
        exit(EXIT_SUCCESS);
    }

    umask(027); /* Set file permissions 750 */

    /* First child get a new process group */
    sid = setsid();
    if (sid < 0) {
      printf("Could not create session for new process group.\n");
      return false;
    }
    /*Second Fork*/
    pid = fork();
    if (pid < 0) {
      /* Could not fork */
      printf("Could not fork() child process\n");
      return false;
    }

    if (pid > 0) {
      /*Second child created ok, so exit first child process */
      exit(EXIT_SUCCESS);
    }
  }

  signal(SIGTERM, signal_handler);
  signal(SIGINT, signal_handler);
  signal(SIGUSR1, signal_handler);
  signal(SIGPIPE, SIG_IGN);
  signal(SIGHUP, SIG_IGN);

  return true;
}

int main(int argc, char *argv[])
{                     
  bool run_asdaemon = false, custom_log_level = false;
  int level = (int)LL_ERROR;
  std::string config_file = "", prefix = "";
  u_int size_info_data = 0;
  int offset_seek_info = 0;
  bool enable_qa = false, clean_devinfo = false, write_devinfo = false, echoinfo = false;

  static struct option long_options[] = {
      {"log", required_argument, NULL, 'l'},
      {"daemon", no_argument, NULL, 'd'},
      {"prefix", no_argument, NULL, 'p'},
      {"help", no_argument, NULL, 'h'},
      {"version", no_argument, NULL, 'v'},
      {NULL, 0, NULL, 0}};
  const char *argument_template = "l:dhvf:p:t:";
  int opt, opt_idx = 0;
  while (-1 != (opt = getopt_long(argc, argv, argument_template, long_options, &opt_idx))) {
    if ('d' == opt) run_asdaemon = true;
    else if ('l' == opt) {
      level = atoi(optarg);
      if (level < 0) level = 0;
      else if (level > (int)LL_VERBOSE) level = (int)LL_DEBUG;

      custom_log_level = true;
    } else if ('p' == opt) {
      prefix = optarg;
    } else if ('h' == opt) {
      printf("Usage: %s [-h|--help] [-l|--log 7] [-d|--daemon] [-p|prefix 25edbba5bce6]\n", argv[0]);
      return 0;
    } else if ('v' == opt) {
      printf("HIREFS version %s\n", HIRE_PRODUCT_VERSION);
      return 0;
    } else if ('t' == opt) {
      u_char buffer[65536];
      u_int len = 65536;
      auxHelper::hexstring2Byte(std::string(optarg), buffer, len);
      printf("\nRESULT : \n%s\n", (char *)buffer);
      return 0;
    }
  }

  daemonize(run_asdaemon);

  char product_name[64] = {0};
  snprintf(product_name, sizeof product_name, "%s%s", PRODUCT_NAME, prefix.c_str());

  char pidPath[64] = {0};
  //check instance
  snprintf(pidPath, sizeof pidPath, "%s/%s.pid", TEMP_DIR, product_name);
  if (false == check_instance(pidPath)) exit(EXIT_FAILURE);

  if (run_asdaemon) { 
    printf("Run %s success.\n", product_name);

    /* close all descriptors */
    //for (int i = getdtablesize(); i >= 0; --i)
    // android not support getdtablesize
    for (int i = STDERR_FILENO; i >= STDIN_FILENO; --i)  
      close(i);
    
    /* Route I/O connections to /dev/null
     * we will be using syslog for messaging
     */
    int stdi = open("/dev/null", O_RDWR); /* Open STDIN */
    dup2(stdi, STDIN_FILENO);             /* STDIN */
    dup2(stdi, STDOUT_FILENO);            /* STDOUT */
    dup2(stdi, STDERR_FILENO);            /* STDERR */
  }

  //log
  static loggerIO log_local;
  logger::instance().set_working_directory(LOG_DIR);
  logger::instance().set_prefix(product_name);
  logger::instance().set_IOObj(&log_local);
  if (true == custom_log_level) {
    printf("log level %d\n", level);
    logger::instance().setDebugModel();
    logger::instance().setLevel((LOG_LEVEL)level);
  } else logger::instance().setLevel(LL_INFO);
  
  struct stat st;
  if (0 != stat(HIREFS_VOLUME_DIRECTORY, &st))
    auxHelper::System("mkdir -p %s", HIREFS_VOLUME_DIRECTORY);

  hireManager hire_manager;
  
  hire_manager.load_hire_config(config_file.empty() ?  HIREFS_FILE_NAME : config_file);

  hire_manager.start();

  while (!bMainExit) sleep(1);

  hire_manager.stop();
  hire_manager.wait_stop();
    
  unlink(pidPath);

  debugEntry(LL_INFO, LOG_MODULE_INDEX_HIRE, "%s stopped.", product_name);
  
  return 0;
}

