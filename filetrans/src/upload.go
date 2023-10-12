package main

import (
	"encoding/json"
	"fmt"
	"io"
	"log"
	"net/http"
	"os"
	"path/filepath"
	"strings"
	"time"

	"github.com/gin-gonic/gin"
	"github.com/tus/tusd/pkg/handler"
	"github.com/tv42/httpunix"
)

func copyHeader(dst, src http.Header) {
	for k, vv := range src {
		for _, v := range vv {
			dst.Add(k, v)
		}
	}
}

func UploadFile(c *gin.Context) {
	req := c.Request
	u := &httpunix.Transport{
		DialTimeout:           10 * time.Second,
		RequestTimeout:        10 * time.Second,
		ResponseHeaderTimeout: 10 * time.Second,
	}
	space := c.Query("space")
	u.RegisterLocation(space, filepath.Join("/tmp", space+".sock"))
	req.URL.Host = space
	req.URL.Scheme = "http+unix"
	req.URL.Host = space
	req.URL.Path = strings.Replace(req.URL.Path, "/v1/file/upload", "/files", -1)

	resp, err := u.RoundTrip(req)
	if err != nil {
		log.Println("err:", err)
		c.JSON(http.StatusServiceUnavailable, gin.H{"msg": err.Error()})
		return
	}

	// 修改Location
	location := resp.Header.Get("Location")
	newLoc := strings.Replace(location, "/files", "/v1/file/upload", -1)
	newLoc += "?space=" + space
	resp.Header.Set("Location", newLoc)
	fmt.Println("err:", resp.Header)
	copyHeader(c.Writer.Header(), resp.Header)
	c.Header("Access-Control-Allow-Origin", "*")

	defer resp.Body.Close()
	io.Copy(c.Writer, resp.Body)
}

func createNewFileName(dir string, name string, isDir bool) string {
	newName := name
	i := 0
	for {
		_, err := os.Stat(filepath.Join(dir, newName))
		if err != nil {
			if os.IsNotExist(err) {
				return newName
			}
		}
		i++
		if isDir {
			newName = fmt.Sprintf("%s_%d", name, i)
		} else {
			filesuffix := filepath.Ext(name)
			fileprefix := name[0 : len(name)-len(filesuffix)]
			newName = fmt.Sprintf("%s_%d%s", fileprefix, i, filesuffix)
		}
	}
}

func HandlerTusd(c *gin.Context) {
	var event handler.HookEvent
	if err := c.ShouldBind(&event); err != nil {
		log.Println("err:", err)
		c.Status(http.StatusBadRequest)
		return
	}
	data, err := json.Marshal(event)
	//	if err == nil {
	log.Println("info:", string(data), err)
	//	}
	if event.Upload.Size == event.Upload.Offset {
		targetPath, ok := event.Upload.MetaData["targetPath"]
		if !ok {
			log.Println("no metadata found")
			c.Status(http.StatusBadRequest)
			return
		}
		filename := event.Upload.MetaData["filename"]
		dir := filepath.Join(app.MountDir, targetPath)
		filename = createNewFileName(dir, filename, false)
		if err := os.Rename(event.Upload.Storage["Path"], filepath.Join(dir, filename)); err != nil {
			log.Println("err:", err)
		}
	}
	c.Status(http.StatusOK)
}
