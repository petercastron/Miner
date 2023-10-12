package main

import (
	"gorm.io/gorm"
)

type Version struct {
	ID      int
	BuildAt int64
}

type VersionFunc func(db *gorm.DB) error
type Versions []struct {
	Time    int64
	Upgrade VersionFunc
}

var versions = Versions{
	{Time: 1667961528, Upgrade: FirstMigrate},
}

// InitVersion 上一个版本， 当前版本
func InitVersion(db *gorm.DB) error {
	db.AutoMigrate(&Version{})
	var foundVers []Version
	db.Find(&foundVers)
	for _, version := range versions {
		found := false
		for _, foundVer := range foundVers {
			if version.Time == foundVer.BuildAt {
				found = true
				break
			}
		}
		if !found {
			if err := version.Upgrade(db); err != nil {
				return err
			}
			db.Create(&Version{BuildAt: version.Time})
		}
	}
	return nil
}

// FirstMigrate update
func FirstMigrate(db *gorm.DB) error {
	return db.Debug().AutoMigrate(&RentalInfo{})
}
