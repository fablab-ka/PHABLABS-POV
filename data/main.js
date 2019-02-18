// XMLHttpRequest

if (typeof XMLHttpRequest === "undefined") {
    XMLHttpRequest = function() {
        try { return new ActiveXObject("Msxml2.XMLHTTP.6.0"); } catch (e) {}
        try { return new ActiveXObject("Msxml2.XMLHTTP.3.0"); } catch (e) {}
        try { return new ActiveXObject("Microsoft.XMLHTTP"); } catch (e) {}
        throw new Error("This browser does not support XMLHttpRequest.");
    };
}

function doGetRequest(_url, _func_onReady, requestIsBinary) {
    requestIsBinary = requestIsBinary || false;
    var xhr = new XMLHttpRequest();
    xhr.open("GET", _url, true);
    if (requestIsBinary) { 
      xhr.responseType = "arraybuffer"; 
    } else {
      xhr.responseType = "text"; 
    }
    xhr.onreadystatechange = function() {
        if (xhr.readyState == 4 && xhr.status == 200) {
            if (requestIsBinary) {
                var ByteArray = new Uint8Array(xhr.response);
                _func_onReady(ByteArray);
            } else {
                _func_onReady(xhr.responseText);
            }
        }
    };
    xhr.send(null);
}

function replaceAjaxDivs() {
  let replaceDivs = document.querySelectorAll("div[id^=AJAXLOAD-]");
  for (let i = 0; i < replaceDivs.length; i++) {
      let currDiv=replaceDivs[i];
      let myId = currDiv.id;
      let htmlFile = "."+myId.match(/^AJAXLOAD-(.*)$/)[1]+".html";
      let newDiv=document.createElement('div');
      doGetRequest(htmlFile, function(_responseText){newDiv.innerHTML=_responseText; currDiv.parentNode.replaceChild(newDiv, currDiv);});
  }
}


// Websockets

class  socketHandler  {
  constructor(socketUri ) {
    this._myself=this;
    this._uri=socketUri;
    this._connectNewSocket();
    this._connectionCounter=0;
    setInterval(this._checkSocket.bind(this), 3000);
  }
  _connectNewSocket() {
    this._socket = new WebSocket(this._uri);
    if (this._socket !== null) {
      this._socket.onopen = function(e) {
        this._connectionCounter++;
        if (typeof this._onOpen === "function") {
          this._onOpen(e);
        }  
      }.bind(this)   
      this._socket.onmessage = function(e) {
        if (typeof this._onMessage === "function") {
          this._onMessage(e);
        }  
      }.bind(this) 
      this._socket.onclose = function(e) {
        if (typeof this._onClose === "function") {
          this._onClose(e);
        }  
        this._checkSocket();
      }.bind(this) 
      this._socket.onerror = function(e) {
        if (typeof this._onError === "function") {
          this._onError(e);
        }  
        this._checkSocket();
      }.bind(this) 
    }
  }
  _checkSocket() {
    if(!this._socket || this._socket.readyState == 3) {
      this._connectNewSocket();
    }  
  } 
  send(message) {
    if (this._socket.readyState == 1) {
      this._socket.send(message);
      return true; 
      /* true does not mean, that the transmission was succesful!
         e.g. when the server has restarted and the clients tcp connection is still open
         the timeout after send triggers an error condition*/
    } else {
      return false;
    }  
  }
  addOnOpen(func) {
    this._onOpen=func;
  }
  addOnMessage(func) {
    this._onMessage=func;
  }
  addOnClose(func) {
    this._onClose=func;
  }
  addOnError(func) {
    this._onError=func;
  }
}  

