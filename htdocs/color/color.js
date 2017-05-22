
/*
window.onload = function() {
  colorPicker();
}
*/

window.addEventListener("load", function(event) {  
	                              colorPicker();
                                //onoff_active(false);
								                getStatus(); 
							                }, false);

function ismobile() { 
  if( navigator.userAgent.match(/Android/i)
   || navigator.userAgent.match(/webOS/i)
   || navigator.userAgent.match(/iPhone/i)
   || navigator.userAgent.match(/iPad/i)
   || navigator.userAgent.match(/iPod/i)
   || navigator.userAgent.match(/BlackBerry/i)
   || navigator.userAgent.match(/Windows Phone/i))
  {
    //alert("Mobile");
    return true;
  } else {
    //alert("Not Mobile");
    return false;
  }
}

function resetColorpicker() {
  document.getElementById('Rslider').value = 0;
  document.getElementById('Gslider').value = 0;
  document.getElementById('Bslider').value = 0;
  var label = document.getElementById('result');
  label.innerHTML = '&nbsp;';
  //label.style.backgroundColor = "#000000";
  label.style.backgroundColor = 'transparent';
  //alert("hide");
  //document.getElementById('Speed').style.visibile = 'hidden';
}

function setColorpickerStatus(active) {
  if (active == true) {    
    //document.getElementById("sslider_div").style.display = 'none';
    document.getElementById("sslider_div").style.opacity = '0.3';
    document.getElementById('Sslider').disabled = true;
    //document.getElementById("sslider_div").style.pointer-events = none; 
  } else {
    resetColorpicker();
    //document.getElementById("sslider_div").style.display = '';
    document.getElementById("sslider_div").style.opacity = '1';
    document.getElementById('Sslider').disabled = false;
  }
}

function onoff_active(status) {
  var onoff = document.getElementById("onoff");
  //if (onoff.src.endsWith("on.png") == true) {
  if (status == true && onoff.src.endsWith("on.png") != true)
    onoff.src="../on.png";
  else if (status == false && (onoff.src.endsWith("off.png") != true) )
    onoff.src="../off.png";
}

function colorPicker() {
  var mobile = ismobile();
  var canvas = document.getElementById('Cpicker');
  var contex = canvas.getContext('2d');
  var doc = document;
  var label = document.getElementById('result');
  var img = new Image();
  var Rslider = document.getElementById('Rslider');
  var Gslider = document.getElementById('Gslider');
  var Bslider = document.getElementById('Bslider');
  var Sslider = document.getElementById('Sslider');
  var patternSelect = document.getElementById('pattern');
  var onoff = document.getElementById('onoff');
  //var rPC, gPC, bPC;
  //rPC = gPC = bPC = 0;

  patternSelect.addEventListener("change", function(event) {
                   //resetColorpicker();
                   //rPC = gPC = bPC = 0;
                   
                   Sslider.value = 50;
                   sendColor();
                   
                   if (patternSelect.value == 0)
                     setColorpickerStatus(true);
                   else {
                     setColorpickerStatus(false);
                     setLabel(label, 0, 0, 0);
                   }
                   
                 }, false);
  
  //alert(canvas.width+", "+canvas.height);
  //document.getElementById('result').innerHTML = canvas.width+", "+canvas.height;
  
  img.src = 'colorwheel.png';
  img.onload = function() {
    contex.drawImage(img, 0, 0, canvas.width, canvas.height);
  };

  Rslider.oninput = Gslider.oninput = Bslider.oninput = function() {
    redraw(Rslider.value, Gslider.value, Bslider.value, true)
  };
  
  Sslider.oninput = function() {
    if ( patternSelect.value > 0) {
      sendColor();
    } /*else {
      setLabel(label, 0, 0, 0);
    }*/
    setLabel(label, 0, 0, 0);
  };

  var diameter = canvas.width;
  var circleOffset = 0;
  var radius = diameter / 2;
  var radiusSquared = radius * radius;
  var radiusPlusOffset = radius + circleOffset;


  //canvas.addEventListener('mousemove', function(event) {};
  
  canvas.addEventListener('mousemove', function(event) {
    setRGBfromcanvas(event, event.clientX, event.clientY);
  });
  
  canvas.addEventListener('click',function(event) {setRGBfromcanvas(event, event.clientX, event.clientY);});
  
  onoff.addEventListener('click',function(event) {toggleOnOff();});
  
  if (mobile == false) {
    var ignoreMouse = true;
    canvas.addEventListener('mousedown',function(event) {ignoreMouse = false;});
    canvas.addEventListener('mouseup',function(event) {ignoreMouse = true;});
  }

  canvas.addEventListener("touchmove", function(event) {
    setRGBfromcanvas(event, event.touches[0].pageX, event.touches[0].pageY);
  });
  //canvas.addEventListener("touchstart", function (e) {}
  //canvas.addEventListener("touchend", function (e) {}

  function toggleOnOff() {
    if (onoff.src.endsWith("on.png") == true) {
      patternSelect.value = Rslider.value = Gslider.value = Bslider.value = 0;
      Sslider.value = 50;
      sendColor();
      setLabel(label, 0, 0, 0);
      //onoff.src = "../off.png"; // Let the server reply set the image
    } else {
    	//onoff.src = "../on.png";
      Rslider.value = Math.floor((Math.random() * 255) + 1);
      Gslider.value = Math.floor((Math.random() * 255) + 1);
      Bslider.value = Math.floor((Math.random() * 255) + 1);
      sendColor();
      setLabel(label, Rslider.value, Gslider.value, Bslider.value);
    }
  }
  
  function setRGBfromcanvas(event, posX, posY) {
    //console.log("Event which="+event.which);
    //if (mobile == false && event.which != 1) // Check the mouse is down(clicked) on desktop browser
    //  return;    
    if (mobile == false && ignoreMouse == true)
      return;

    //rPC = gPC = bPC = 0;
        
    var rect = canvas.getBoundingClientRect();
    var x = posX - rect.left;
    var y = posY - rect.top;

    currentX = (x - radius);
    currentY = (y - radius);
    var theta = Math.atan2(currentY, currentX),
      d = currentX * currentX + currentY * currentY;

    if (d < radiusSquared) {
      var imgd = contex.getImageData(x, y, 1, 1);
      var data = imgd.data;
      redraw(data[0], data[1], data[2], false);
    } else {
      //console.log("Mouse outside circle");
      //label.innerHTML = "Mouse outside circle";
    }
  };

  // Prevent scrolling when touching the canvas on mobile 
  document.body.addEventListener("touchstart", function(e) {
    if (e.target == canvas) {
      e.preventDefault();
    }
  }, false);
  document.body.addEventListener("touchend", function(e) {
    if (e.target == canvas) {
      e.preventDefault();
    }
  }, false);
  document.body.addEventListener("touchmove", function(e) {
    if (e.target == canvas) {
      e.preventDefault();
    }
  }, false);


  function redraw(r, g, b, fromSlider) {
    if (fromSlider != true && Rslider.value == r && Gslider.value == g &&  Bslider.value == b)
      return;  // If nothing changed, return. (duplicate events come from mousemove & mouseclick)
    
    setColorpickerStatus(true);
    
    setLabel(label, r, g, b);
    
    Rslider.value = r;
    Gslider.value = g;
    Bslider.value = b;
       
    document.getElementById('pattern').value = 0;
    document.getElementById('Sslider').value = 50;
    sendColor('&r='+r+'&g='+g+'&b='+b);
     
    //console.log("R:" + r + " G:" + g + " B:" + b);
  }
}

function setLabel(label, r, g, b) {
  
  if (r == 0 && g == 0 && b == 0) {
    label.style.backgroundColor = 'transparent';
    label.style.color = "#ffffff";
    var pattern = document.getElementById('pattern');
    if ( pattern.value > 0) {
      label.innerHTML = "<font size=\"-1\">" + pattern.options[pattern.selectedIndex].innerHTML + "</font><font size=\"-3\"><br>speed "+(100 - document.getElementById('Sslider').value)+ "</font>";
    } else {
      label.innerHTML = '&nbsp;';   
    }
    return;
  }
  
    label.innerHTML = '&nbsp;&nbsp;R:'+r+'&nbsp;G:'+g+'&nbsp;B:'+b+"&nbsp;&nbsp;";
    var hexString = RGBtoHex(r, g, b);
    label.style.backgroundColor = "#" + hexString;
    if ((r + g + b) > 600)
      label.style.color = "#000000";
    else
      label.style.color = "#ffffff";
}

function RGBtoHex(R,G,B) {return toHex(R)+toHex(G)+toHex(B)};

function toHex(N) {
      if (N==null) return "00";
      N=parseInt(N); if (N==0 || isNaN(N)) return "00";
      N=Math.max(0,N); N=Math.min(N,255); N=Math.round(N);
      return "0123456789ABCDEF".charAt((N-N%16)/16)
           + "0123456789ABCDEF".charAt(N%16);
};

function sendColor(rgb) {
  
  if (rgb == null) {
    rgb = "r="+document.getElementById('Rslider').value+"&g="+document.getElementById('Gslider').value+"&b="+document.getElementById('Bslider').value+""
    //reset();
  } else {
    document.getElementById('pattern').value = 0;
    document.getElementById('Sslider').value = 50;
  }
  
  var $http;
  $self = arguments.callee;
  if (window.XMLHttpRequest) {
    $http = new XMLHttpRequest();
  } else if (window.ActiveXObject) {
    try {
      $http = new ActiveXObject('Msxml2.XMLHTTP');
    } catch (e) {
      $http = new ActiveXObject('Microsoft.XMLHTTP');
    }
  }
  if ($http) {
    $http.onreadystatechange = function() {
      if (/4|^complete$/.test($http.readyState)) {
        var data = JSON.parse($http.responseText);
        //console.log("Receive : R=" + data.led.r + " G=" + data.led.g + " B=" + data.led.b + " Pattern=" + data.led.p);
        document.getElementById('pattern').value = data.led.p;
        
        if (data.led.p > 0 || data.led.r > 0 || data.led.g > 0 || data.led.b > 0)
          onoff_active(true);
        else
          onoff_active(false);
      }
    }
  };
  console.log('GET', location.protocol + '//' + location.host + '/led?' + rgb + '&p=' + document.getElementById('pattern').value + '&o=' +   document.getElementById('Sslider').value);
  $http.open('GET', location.protocol + '//' + location.host + '/led?' + rgb + '&p=' + document.getElementById('pattern').value + '&o=' +  document.getElementById('Sslider').value, true);
  $http.send(null);
};

function getStatus() {
  var $http;
  $self = arguments.callee;
  if (window.XMLHttpRequest) {
    $http = new XMLHttpRequest();
  } else if (window.ActiveXObject) {
    try {
      $http = new ActiveXObject('Msxml2.XMLHTTP');
    } catch (e) {
      $http = new ActiveXObject('Microsoft.XMLHTTP');
    }
  }
  if ($http) {
    $http.onreadystatechange = function() {
      if (/4|^complete$/.test($http.readyState)) {
        var data = JSON.parse($http.responseText);
        //console.log("Receive Status: R=" + data.led.r + " G=" + data.led.g + " B=" + data.led.b + " Pattern=" + data.led.p);
        document.getElementById('pattern').value = data.led.p;
        document.getElementById('Rslider').value = data.led.r;
        document.getElementById('Gslider').value = data.led.g;
        document.getElementById('Bslider').value = data.led.b;
        document.getElementById('Sslider').value = data.led.o;
        
        if (data.led.p == 0 /*&& data.led.o == 0*/) {
          document.getElementById('Sslider').value = 50;
          setColorpickerStatus(true);
        } else {
          document.getElementById('Sslider').value = data.led.o;
          setColorpickerStatus(false);
        }
        
        setLabel(document.getElementById('result'), data.led.r, data.led.g, data.led.b);
        
        if (data.led.p > 0 || data.led.r > 0 || data.led.g > 0 || data.led.b > 0)
          onoff_active(true);
        else
          onoff_active(false);
      }
    }
  };
  $http.open('GET', location.protocol + '//' + location.host + '/led?a=getstatus', true);
  $http.send(null);
};