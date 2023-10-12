package main

import (
	"crypto/sha256"
	"fmt"
	"log"
	"math/rand"
	"net/http"
	"os"
	"path/filepath"
	"time"

	"github.com/comail/colog"
	"github.com/gin-contrib/cors"
	"github.com/gin-gonic/gin"
)

var mode string

func init() {
	rand.Seed(time.Now().UnixNano())
}

func main() {
	colog.Register()
	colog.SetFlags(log.LstdFlags | log.Lshortfile)
	if len(mode) == 0 {
		mode = "release"
	}
	log.Println("mode", mode)
	binPath, err := os.Executable()
	if err != nil {
		log.Println(err)
	}
	path, err := os.Getwd()
	if err != nil {
		log.Println(err)
	}
	log.Println("info: current directory", path, binPath)
	binDir := filepath.Dir(binPath)

	InitApp()

	r := gin.Default()
	authMiddleware, err := AuthInit()
	if err != nil {
		log.Fatal("err:", err.Error())
	}
	// CORS
	config := cors.DefaultConfig()
	config.AllowAllOrigins = true
	config.AddAllowHeaders("*")
	config.AllowWildcard = true
	r.Use(cors.New(config))
	r.POST("/login", authMiddleware.LoginHandler)
	r.GET("/refresh_token", authMiddleware.RefreshHandler)
	//	r.Any("/tusd", HandlerTusd)
	r.GET("/hello", func(ctx *gin.Context) {
		randStr := fmt.Sprintf("%d:%f:%f:%f", time.Now().UnixNano(), rand.Float64(), rand.Float64(), rand.Float64())
		h := sha256.New()
		h.Write([]byte(randStr))
		bs := h.Sum(nil)
		challenge := fmt.Sprintf("%x", bs)
		app.Lock()
		app.Challenges[challenge] = struct{}{}
		app.Unlock()
		ctx.JSON(http.StatusOK, gin.H{"challenge": challenge})
	})

	file := r.Group("/v1/file").Use(authMiddleware.MiddlewareFunc())
	{
		file.GET("/list", ListFile)
		file.POST("/create", Mkdir)
		file.POST("/delete", DeleteFile)
		file.GET("/download", DownloadFile)
		file.GET("/spaces", Spaces)
		file.Any("/upload", UploadFile)
		file.Any("/upload/:uploadId", UploadFile)
		file.POST("/clearCache", ClearCache)
	}

	t2 := gin.Default()
	t2.Any("/tusd", HandlerTusd)
	var utgServer *gin.RouterGroup
	if mode != "release" {
		utgServer = r.Group("/v1/utg")
	} else {
		utgServer = t2.Group("/v1/utg")

	}
	utgServer.POST("/rent", Rent)
	utgServer.POST("/free", Free)

	go t2.Run("localhost:8081")

	r.StaticFS("/static", http.Dir(filepath.Join(binDir, "web/static")))
	r.LoadHTMLFiles(filepath.Join(binDir, "web/index.html"))
	r.GET("/home", func(c *gin.Context) {
		c.HTML(http.StatusOK, "index.html", gin.H{})
	})

	r.Run()
}
