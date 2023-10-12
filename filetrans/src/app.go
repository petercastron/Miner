package main

import (
	"flag"
	"fmt"
	"log"
	"os"
	"os/exec"
	"path/filepath"
	"sync"

	"gorm.io/driver/sqlite"
	"gorm.io/gorm"
)

type App struct {
	sync.Mutex
	DB         *gorm.DB
	DataDir    string
	MountDir   string
	Port       string
	Challenges map[string]struct{}
}

var app App

func InitApp() {
	flag.StringVar(&app.Port, "port", "8080", "Port to bind HTTP server to")
	flag.StringVar(&app.DataDir, "data-dir", "/sata", "Directory to allocate disk space")
	flag.StringVar(&app.MountDir, "mount-dir", "/tmp", "Directory to mount")
	flag.Parse()
	os.MkdirAll(app.DataDir, 0755)
	os.MkdirAll(app.MountDir, 0755)
	os.MkdirAll("/tmp/empty_dir", 0755)
	dbDir := filepath.Join(app.DataDir, ".utg")
	os.Mkdir(dbDir, 0755)
	path := filepath.Join(dbDir, "data.db")
	urn := fmt.Sprintf("file:%s?cache=shared&_journal_mode=WAL", path)
	var err error
	app.DB, err = gorm.Open(sqlite.Open(urn), &gorm.Config{})
	if err != nil {
		log.Fatal("err:", err)
	}
	if err := InitVersion(app.DB); err != nil {
		log.Fatal("err:", err)
	}
	app.DB.Exec("PRAGMA foreign_keys=ON")
	app.DB.Exec("PRAGMA synchronous=OFF")
	app.Challenges = make(map[string]struct{})

	initMount()
}

func initMount() {
	exec.Command("sh", "-c", "kill `pidof tusd`").Run()
	var rentalInfos []RentalInfo
	app.DB.Find(&rentalInfos)
	for _, rentInfo := range rentalInfos {
		if !checkMoreLoopDev() {
			log.Fatal("err:", "no more loop dev")
		}
		mountpoint := filepath.Join(app.MountDir, rentInfo.Root)
		_, e := os.Lstat(mountpoint)
		if e != nil && os.IsNotExist(e) {
			os.MkdirAll(mountpoint, 0664)
		}
		err := exec.Command("mountpoint", "-q", mountpoint).Run()
		if err != nil {
			output, err := exec.
				Command("mount", "-o", "loop",
					filepath.Join(app.DataDir, rentInfo.Root), mountpoint).
				CombinedOutput()
			if err != nil {
				log.Fatal("err:", string(output))
			}
		}
		unixAddr := filepath.Join("/tmp", rentInfo.Root+".sock")
		pidFile := filepath.Join("/tmp", rentInfo.Root+".pid")
		uploadDir := filepath.Join(mountpoint, ".uploads")
		if err := startTusd(uploadDir, unixAddr, pidFile); err != nil {
			log.Fatal("err:", err)
		}
	}
}
