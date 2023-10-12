build:
	cd ../web && npm run build && cd -
	go mod tidy 
	go build -ldflags "-X main.mode=Release" -o ../dist/utg/utg
	rsync -r --exclude=.git ../web/dist/ ../dist/utg/web/
	cd ../dist \
	tar cvfz utg.tar.gz utg
