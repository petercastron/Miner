export default class WebRTC {
	constructor(iceServers, signalingURL, remoteUUID, onConnect, onProgress, onComplete) {
		if (typeof(iceServers) == "string") {
			iceServers = [{
				urls: iceServers
			}]
		}
		this._pc = new RTCPeerConnection({
			iceServers
		})
		this._pc.onicecandidate = e => {
			if (e.candidate) {
				this.sendto({
					type: "candidate",
					candidate: e.candidate.candidate
				})
			}
		}
		const dc = this._pc.createDataChannel(remoteUUID)
		dc.onopen = () => {
			this._channelOpened = true
		}
		var buffers = []
		var loaded = 0
		dc.onmessage = e => {
			if (e.data.byteLength > 0) {
				buffers.push(e.data)
				loaded += e.data.byteLength
				onProgress(loaded)
			} else {
				dc.send("ok")
				onComplete(buffers)
			}
		}
		const clientUUID = Date.now() + "-" + Math.floor(Math.random() * 0xFFFFFFFF)
		this._ws = new WebSocket(signalingURL)
		this._ws.onopen = () => {
			this.send("connect", clientUUID)
		}
		this._ws.onmessage = e => {
			var result = JSON.parse(e.data)
			switch (result._method) {
				case "connect":
					console.log("connected")
					onConnect(clientUUID)
					break
				case "sendto":
					switch (result.data.type) {
						case "offer":
							this._pc.setRemoteDescription(result.data)
							this._pc.createAnswer().then(answer => {
								this._pc.setLocalDescription(answer)
								this.sendto(answer)
							}).catch(e => {
								console.error(e)
							})
							break
						case "candidate":
							this._pc.addIceCandidate(new RTCIceCandidate(result.data.candidate))
							break
					}
					break
			}
		}

		this._remoteUUID = remoteUUID;
	}

	sendto(data) {
		this._ws.send(JSON.stringify({
			_method: "sendto",
			to: this._remoteUUID,
			data: data
		}))
	}

	send(method, data) {
		this._ws.send(JSON.stringify({
			_method: method,
			data: data
		}))
	}

	isChannelOpened() {
		return this._channelOpened
	}

	close() {
		this._pc.close()
		this._ws.close()
	}
}
