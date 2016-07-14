var Clay = require('pebble-clay');
var clayConfig = require('./config');
var clay = new Clay(clayConfig);

/*
* KiezelPay Integration Library - v1.5 - Copyright Kiezel 2016
*
* BECAUSE THE LIBRARY IS LICENSED FREE OF CHARGE, THERE IS NO 
* WARRANTY FOR THE LIBRARY, TO THE EXTENT PERMITTED BY APPLICABLE 
* LAW. EXCEPT WHEN OTHERWISE STATED IN WRITING THE COPYRIGHT 
* HOLDERS AND/OR OTHER PARTIES PROVIDE THE LIBRARY "AS IS" 
* WITHOUT WARRANTY OF ANY KIND, EITHER EXPRESSED OR IMPLIED, 
* INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF 
* MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE. THE ENTIRE
* RISK AS TO THE QUALITY AND PERFORMANCE OF THE LIBRARY IS WITH YOU.
* SHOULD THE LIBRARY PROVE DEFECTIVE, YOU ASSUME THE COST OF ALL 
* NECESSARY SERVICING, REPAIR OR CORRECTION.
* 
* IN NO EVENT UNLESS REQUIRED BY APPLICABLE LAW OR AGREED TO IN 
* WRITING WILL ANY COPYRIGHT HOLDER, OR ANY OTHER PARTY WHO MAY 
* MODIFY AND/OR REDISTRIBUTE THE LIBRARY AS PERMITTED ABOVE, BE 
* LIABLE TO YOU FOR DAMAGES, INCLUDING ANY GENERAL, SPECIAL, 
* INCIDENTAL OR CONSEQUENTIAL DAMAGES ARISING OUT OF THE USE OR 
* INABILITY TO USE THE LIBRARY (INCLUDING BUT NOT LIMITED TO LOSS
* OF DATA OR DATA BEING RENDERED INACCURATE OR LOSSES SUSTAINED BY 
* YOU OR THIRD PARTIES OR A FAILURE OF THE LIBRARY TO OPERATE WITH
* ANY OTHER SOFTWARE), EVEN IF SUCH HOLDER OR OTHER PARTY HAS BEEN 
* ADVISED OF THE POSSIBILITY OF SUCH DAMAGES.
*/

require('message_keys');

/* === KIEZELPAY === SET TO false BEFORE RELEASING === */
var KIEZELPAY_LOGGING = true;
/* === KIEZELPAY === SET TO false BEFORE RELEASING === */


/**********************************************************************/
/* === KIEZELPAY === GENERATED CODE BEGIN === DO NOT MODIFY BELOW === */
/**********************************************************************/
var kiezelPayAppMessageHandler = null;

function kiezelPayInit(appMessageHandler) {
  kiezelPayLog("kiezelPayInit() called");
  kiezelPayAppMessageHandler = appMessageHandler;
  
  var msg = {
    KIEZELPAY_READY: 1
  };
  Pebble.sendAppMessage(msg, 
                        function(e) {
                          kiezelPayLog("KiezelPay Ready msg successfully sent.");
                        },
                        function(e) {
                          kiezelPayLog("KiezelPay Ready msg failed.");
                        });
}

function kiezelPayLog(msg) {
  if (KIEZELPAY_LOGGING) {
    console.log(msg);
  }
}

var xhrRequest = function (url, type, callback, errorCallback, timeout) {
  try {
    var xhr = new XMLHttpRequest();
    xhr.onload = function () {
      callback(this.responseText);
    };
    xhr.open(type, url);
    if (timeout) {
      xhr.timeout = timeout;
    }
    xhr.send();
  }
  catch (ex) {
    if (errorCallback) {
      errorCallback(ex);
    }
  }
};

function kiezelPayOnAppMessage(appmsg) {
  if (appmsg && appmsg.payload && 
      appmsg.payload.hasOwnProperty('KIEZELPAY_STATUS_CHECK') && 
      appmsg.payload.KIEZELPAY_STATUS_CHECK > 0) {
    //its a status check message, handle it
    var devicetoken = appmsg.payload.KIEZELPAY_DEVICE_TOKEN;
    var appId = appmsg.payload.KIEZELPAY_APP_ID;
    var random = appmsg.payload.KIEZELPAY_RANDOM;
    var accounttoken = Pebble.getAccountToken();
    var platform = getPlatform();
    
    //build url
    var kiezelpayStatusUrl = 'https://www.kiezelpay.com/api/v1/status?';
    kiezelpayStatusUrl += 'appid=' + encodeURIComponent(appId);
    kiezelpayStatusUrl += '&devicetoken=' + encodeURIComponent(devicetoken);
    kiezelpayStatusUrl += '&rand=' + encodeURIComponent(random);
    kiezelpayStatusUrl += '&accounttoken=' + encodeURIComponent(accounttoken);
    kiezelpayStatusUrl += '&platform=' + encodeURIComponent(platform);
    kiezelpayStatusUrl += '&flags=' + encodeURIComponent(appmsg.payload.KIEZELPAY_STATUS_CHECK);
    kiezelpayStatusUrl += '&nocache=' + encodeURIComponent(Math.round(new Date().getTime()));
    kiezelPayLog(kiezelpayStatusUrl);
    
    //perform the request
    xhrRequest(kiezelpayStatusUrl, "GET", 
      function(responseText) {
        kiezelPayLog("KiezelPay status response: " + responseText);
        var response = JSON.parse(responseText);
        var status = 0;
        var trialDuration = 0;
        var paymentCode = 0;
        var purchaseStatus = 0;
        if (response.status === 'unlicensed') {
          status = 0;
          paymentCode = Number(response.paymentCode);
          if (response.purchaseStatus == 'waitForUser') {
            purchaseStatus = 0;
          }
          else if (response.purchaseStatus == 'inProgress') {
            purchaseStatus = 1;
          }
        } else if (response.status == 'trial') {
          status = 1;
          trialDuration = Number(response.trialDurationInSeconds);
        } else if (response.status == 'licensed') {
          status = 2;
        }

        var msg = {
          KIEZELPAY_STATUS_RESULT: status,
          KIEZELPAY_STATUS_TRIAL_DURATION: trialDuration,
          KIEZELPAY_PURCHASE_CODE: paymentCode,
          KIEZELPAY_PURCHASE_STATUS: purchaseStatus,
          KIEZELPAY_STATUS_VALIDITY_PERIOD: response.validityPeriodInDays,
          KIEZELPAY_STATUS_CHECKSUM: kiezelpay_toByteArray(response.checksum)
        };
        kiezelPayLog("KiezelPay watch status msg: " + JSON.stringify(msg));
        Pebble.sendAppMessage(msg,
                              function (e) {
                                kiezelPayLog('KiezelPay status msg successfully sent to watch');
                              },
                              function (e) {
                                kiezelPayLog('KiezelPay status msg failed sending to watch');
                              });
      },
      function (error) {
        kiezelPayLog('KiezelPay status request failed: ' + JSON.stringify(error));
        kiezelpay_sendInternetFailedMsg();
      }, 5000);
    return true;    //its our message, we handled it
  }
  
  return false;    //not a kiezelpay message
}

function kiezelpay_sendInternetFailedMsg() {
  var msg = {
    KIEZELPAY_INTERNET_FAIL: 1
  };
  Pebble.sendAppMessage(msg,
                        function (e) {
                          kiezelPayLog('KiezelPay internet fail successfully sent');
                        },
                        function (e) {
                          kiezelPayLog('KiezelPay internet fail not sent');
                        });
}

function kiezelpay_toByteArray(hexStringValue) {
  var bytes = [];
  for (var i = 0; i < hexStringValue.length; i += 2) {
    bytes.push(parseInt(hexStringValue.substr(i, 2), 16));
  }
  return bytes;
}

function getPlatform() {
  if (Pebble.getActiveWatchInfo) {
    return Pebble.getActiveWatchInfo().platform;
  }
  else {
    return "aplite";
  }
}

Pebble.addEventListener("appmessage", function (e) {
  if (!kiezelPayOnAppMessage(e) && kiezelPayAppMessageHandler) {
    kiezelPayAppMessageHandler(e);
  }
});

/********************************************************************/
/* === KIEZELPAY === GENERATED CODE END === DO NOT MODIFY ABOVE === */
/********************************************************************/

var MESSAGE_TYPE_READY = 100,
    MESSAGE_TYPE_WEATHER = 200;

var openWeatherApiKey = "d558cccbf0cfbcd269a8dba4d58216c8";

var useCelcius = true;

Pebble.addEventListener("ready", function (e) {
  //init kiezelpay and register your own handler to receive appMessages
  kiezelPayInit(onAppMessageReceived);

  //add your own "ready" code here
  Pebble.sendAppMessage(
    {
      MESSAGE_TYPE: MESSAGE_TYPE_READY
    },
    function(e) {
      console.log('JS Ready message sent');
    },
    function(e) {
      console.log('JS Ready message failed to send');
    }
  );
});

function onAppMessageReceived(appMsg) {
  console.log('AppMessage data: ' + appMsg.payload.USE_CELCIUS);
  useCelcius = appMsg.payload.USE_CELCIUS;
  getWeather();
}

//
// My code
//

var xhrRequest = function (url, type, callback) {
  var xhr = new XMLHttpRequest();
  xhr.onload = function () {
    callback(this.responseText);
  };
  xhr.open(type, url);
  xhr.send();
};

function locationSuccess(pos) {
  var url = 'http://api.openweathermap.org/data/2.5/weather?lat=' +
    pos.coords.latitude + '&lon=' + pos.coords.longitude + '&APPID=' + openWeatherApiKey;
  
  // Send request to OpenWeatherMap
  xhrRequest(url, 'GET', 
    function(responseText) {
      // responseText contains a JSON object with weather info
      console.log(responseText);
      var json = JSON.parse(responseText);

      // Temperature in Kelvin requires adjustment
      var temperature = Math.round(json.main.temp - 273.15);
      console.log('Temperature is ' + temperature + ' celcius');
      
      // Fahrenheit conversion
      if (!useCelcius) {
        temperature = (9.0/5.0) * temperature + 32;
        console.log('Converted to ' + temperature + ' fahrenheits');
      }
        
      temperature = Math.round(temperature);
      
      // Conditions
      var conditions = json.weather[0].main;      
      console.log('Conditions are ' + conditions);
      
      // Sunrise & sunset
      var sunriseDate = new Date(1000 * json.sys.sunrise);
      var sunsetDate = new Date(1000 * json.sys.sunset);
      var sunriseStr = ('0' + sunriseDate.getHours()).slice(-2) + ('0' + sunriseDate.getMinutes()).slice(-2);
      var sunsetStr = ('0' + sunsetDate.getHours()).slice(-2) + ('0' + sunsetDate.getMinutes()).slice(-2);
      console.log('Sunrise ' + sunriseStr + ', sunset ' + sunsetStr);
      
      // Send to Pebble
      Pebble.sendAppMessage(
        {
          MESSAGE_TYPE: MESSAGE_TYPE_WEATHER,
          TEMPERATURE: temperature,
          CONDITIONS: json.weather[0].id,
          SUNRISE: parseInt(sunriseStr),
          SUNSET: parseInt(sunsetStr)
        },
        function(e) {
          console.log('Weather info sent to Pebble successfully!');
        },
        function(e) {
          console.log('Error sending weather info to Pebble!');
        }
      );
    }      
  );
}

function locationError(err) {
  console.log('Could not get location with GPS');
}

function getWeather() {
  navigator.geolocation.getCurrentPosition(
    locationSuccess,
    locationError,
    {timeout: 15000, maximumAge: 60000}
  );
}
