package main

import (
	"errors"
	"log"
	"net/http"
	"strings"
	"time"

	"github.com/UltronGlow/UltronGlow-Origin/common"
	jwt "github.com/appleboy/gin-jwt/v2"
	"github.com/gin-gonic/gin"
)

func PayloadFunc(data interface{}) jwt.MapClaims {
	return jwt.MapClaims{jwt.IdentityKey: data}
}

func Authorizator(data interface{}, c *gin.Context) bool {
	c.Set("pubkey", data.(string))
	return true
}

func Unauthorized(c *gin.Context, code int, message string) {
	c.JSON(http.StatusOK, gin.H{
		"code": code,
		"msg":  message,
	})
}

type AuthParam struct {
	Sign      string `json:"sign"`
	Data      string `json:"data"`
	Pubkey    string `json:"pubkey"`
	Challenge string `json:"challenge"`
}

func Authenticator(c *gin.Context) (interface{}, error) {
	var param AuthParam
	if err := c.ShouldBindJSON(&param); err != nil {
		return nil, errors.New("invalid parameter")
	}
	app.Lock()
	if _, ok := app.Challenges[param.Challenge]; !ok {
		app.Unlock()
		return nil, errors.New("invalid challenge")
	} else {
		delete(app.Challenges, param.Challenge)
	}
	app.Unlock()
	data := SighHash([]byte(param.Data))
	if !Verify(common.Bytes2Hex(data), param.Sign, param.Pubkey) {
		return nil, errors.New("auth failed")
	}
	log.Println("info:", param.Pubkey)
	return strings.ToLower(param.Pubkey[2:]), nil
}

func AuthInit() (*jwt.GinJWTMiddleware, error) {
	timeout := 2 * time.Hour
	return jwt.New(&jwt.GinJWTMiddleware{
		Realm:         "test zone",
		Key:           []byte("@X|xuFxnReu~]kxW9YQl"),
		Timeout:       timeout,
		MaxRefresh:    time.Hour,
		PayloadFunc:   PayloadFunc,
		Authenticator: Authenticator,
		Authorizator:  Authorizator,
		Unauthorized:  Unauthorized,
		TokenLookup:   "header: Authorization, query: token, cookie: jwt",
		TokenHeadName: "Bearer",
	})
}

func Cors() gin.HandlerFunc {
	return func(ctx *gin.Context) {
		ctx.Header("Access-Control-Allow-Origin", "*")
		ctx.Header("Access-Control-Allow-Methods", "*")
		ctx.Header("Access-Control-Allow-Headers", "*")
		ctx.Header("Access-Control-Allow-", "*")
		ctx.Header("Access-Control-Allow-Origin", "*")
		ctx.Header("Access-Control-Allow-Origin", "*")
		ctx.Header("Access-Control-Allow-Origin", "*")
		ctx.Header("Access-Control-Allow-Origin", "*")
	}
}
