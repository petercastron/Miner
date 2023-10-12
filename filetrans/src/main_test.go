package main

import (
	"errors"
	"fmt"
	"log"

	"testing"

	"github.com/UltronGlow/UltronGlow-Origin/common"
	"github.com/UltronGlow/UltronGlow-Origin/crypto"
)

func TestA(t *testing.T) {
	key := "0x7A4539Ed8A0b8B4583EAd1e5a3F604e83419f502"
	kyes1 := common.HexToAddress(key).Hex()
	log.Println(kyes1)
}

//测试签名钱包私钥
var testPrivHex = "64b87ff4dfe358dfbf54d9eeec538d8ea9e7e96b3d77cc48a7b441fced793be7"

//测试签名钱包地址
var addr string = "ux7A4539Ed8A0b8B4583EAd1e5a3F604e83419f502"

/*
 * 测试签名方法
 * 入参：原始签名字符串
 * 出参：签名结果，错误信息
 */
func sign(orgstr string) (string, error) {
	key, _ := crypto.HexToECDSA(testPrivHex)
	msg := fmt.Sprintf("\x19Ethereum Signed Message:\n%d%s", len(orgstr), orgstr)
	data := crypto.Keccak256([]byte(msg))
	sig, err := crypto.Sign(data, key)
	if err != nil {
		fmt.Println("Sign failed, err:", err)
		return "", errors.New("Sign failed, err: " + err.Error())
	}
	sig[64] = sig[64] + 27
	return common.Bytes2Hex(sig), nil
}

func TestSign(t *testing.T) {
	msg := "0x313233343536"
	//msg1 := fmt.Sprintf("\x19Ethereum Signed Message:\n%d%s", len([]byte(msg)), []byte(msg))
	//msg1 := fmt.Sprintf("\x19Ethereum Signed Message:\n%d%s", len(msg), msg)
	//data := crypto.Keccak256([]byte(msg1))
	data := SighHash([]byte(msg))
	log.Println(Verify(common.Bytes2Hex(data),
		"d33490d7828fad9ede6808cd4529abf4bad2cf8ae40ca0965ea56e5ddb03dd883147c5a64cb89a8c424f2d20f4faedf78a6b7235e80ffa901ea2cdacd596f0561b",
		"ux8dd0039454A641aaD63D187D8aCCf0fB020d613c"))
}
