var MESSAGE_TYPE_CONFIG = 3;

function parseColorInt(value) {
  return parseInt(value.substr(value.length - 6), 16);
}

Pebble.addEventListener('showConfiguration', function(e) {
  Pebble.openURL('http://95.85.12.164/mytime/');
});

Pebble.addEventListener('webviewclosed', function(e) {
  // Decode and parse config data as JSON
  var config_data = JSON.parse(decodeURIComponent(e.response));
  console.log('Config window returned: ', JSON.stringify(config_data));

  // Send settings to Pebble watchapp
  Pebble.sendAppMessage(
    {
      'KEY_MESSAGE_TYPE': MESSAGE_TYPE_CONFIG,
      'KEY_USE_CELCIUS': parseInt(config_data.use_celcius),
      'KEY_COLOR_BG': parseColorInt(config_data.color_bg),
      'KEY_COLOR_TEXT': parseColorInt(config_data.color_text),
      'KEY_COLOR_LIGHT': parseColorInt(config_data.color_light),
      'KEY_COLOR_DARK': parseColorInt(config_data.color_dark),
      'KEY_COLOR_MINUTE': parseColorInt(config_data.color_minute),
      'KEY_COLOR_HOUR': parseColorInt(config_data.color_hour),
      'KEY_COLOR_ACTIVITY': parseColorInt(config_data.color_activity),
      'KEY_ACTIVITY_SENSITIVITY': parseInt(config_data.activity_sensitivity)
    },
    function(){
      console.log('Sent config data to Pebble');  
    },
    function() {
      console.log('Failed to send config data!');
    }
  );
});

var MESSAGE_TYPE_READY = 1,
    MESSAGE_TYPE_WEATHER = 2,
    MESSAGE_TYPE_CONFIG = 3;

var openWeatherApiKey = "d558cccbf0cfbcd269a8dba4d58216c8";

var useCelcius = true;

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
          'KEY_MESSAGE_TYPE': MESSAGE_TYPE_WEATHER,
          'KEY_TEMPERATURE': temperature,
          'KEY_CONDITIONS': json.weather[0].id,
          'KEY_SUNRISE': parseInt(sunriseStr),
          'KEY_SUNSET': parseInt(sunsetStr)
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

// Listen for when the watchface is opened
Pebble.addEventListener('ready', 
  function(e) {
    Pebble.sendAppMessage(
      {
        'KEY_MESSAGE_TYPE': MESSAGE_TYPE_READY
      },
      function(e) {
        console.log('JS Ready message sent');
      },
      function(e) {
        console.log('JS Ready message failed to send');
      }
    );
  }
);

// Listen for when an AppMessage is received
Pebble.addEventListener('appmessage',
  function(e) {
    console.log('AppMessage data: ' + e.payload.KEY_USE_CELCIUS);
    useCelcius = e.payload.KEY_USE_CELCIUS;
    getWeather();
  }                     
);