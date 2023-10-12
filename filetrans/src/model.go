package main

type RentalInfo struct {
	Voucher   string `json:"voucher"`
	PubKey    string `json:"pubkey"`
	Size      uint64 `json:"size"`
	Root      string `json:"root"`
	Duration  uint64 `json:"duration"`
	Deadline  uint64 `json:"deadline"`
	CreatedAt int64  `json:"created_at"`
	Used      uint64 `json:"used" gorm:"all:-"`
}
