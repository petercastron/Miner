package main

import (
	"log"
	"net/http"
	"path/filepath"

	"github.com/gin-gonic/gin"
)

func DownloadFile(c *gin.Context) {
	path := c.Query("path")
	pubkey := c.GetString("pubkey")
	if err := checkPathValid([]string{path}, pubkey); err != nil {
		log.Println("err:", err)
		c.JSON(http.StatusOK, gin.H{"code": StatusParamInvalid})
		return
	}
	absPath := filepath.Join(app.MountDir, path)
	c.File(absPath)
}
