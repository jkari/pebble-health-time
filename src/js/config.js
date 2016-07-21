module.exports = [
  { 
    "type": "heading", 
    "defaultValue": "Health Time" 
  },
  {
    "type": "text",
    "defaultValue": "<p>Please direct feature requests to my Twitter account @jussikari.</p>"
  },
  {
    "type": "section",
    "items": [
      {
        "type": "heading",
        "defaultValue": "Theme"
      },
      {
        "type": "select",
        "messageKey": "THEME",
        "defaultValue": "light",
        "label": "Theme",
        "options": [
          {
            "label": "Light",
            "value": "light"
          },
          {
            "label": "Dark",
            "value": "dark"
          }
        ]
      }
    ]
  },
  { 
    "type": "section", 
    "items": [
      {
        "type": "heading",
        "defaultValue": "Weather"
      },
      {
        "type": "select",
        "messageKey": "USE_CELCIUS",
        "label": "Temperature unit",
        "defaultValue": "1",
        "options": [
          {
            "label": "Celcius",
            "value": "1"
          },
          {
            "label": "Fahrenheit",
            "value": "0"
          }
        ]
      },
      {
        "id": "weather-provider",
        "type": "select",
        "messageKey": "WEATHER_PROVIDER",
        "label": "Weather provider",
        "defaultValue": "owm",
        "options": [
          {
            "label": "Forecast.io",
            "value": "fio"
          },
          {
            "label": "OpenWeatherMap",
            "value": "owm"
          },
          {
            "label": "Weather Underground",
            "value": "wu"
          }
        ]
      },
      {
        "type": "text",
        "defaultValue": "You need to provide API key for the selected provider."
      },
      {
        "type": "input",
        "messageKey": "APIKEY_FORECASTIO",
        "label": "API Key for Forecast.io"
      },
      {
        "type": "input",
        "messageKey": "APIKEY_OPENWEATHERMAP",
        "label": "API Key for OpenWeatherMap"
      },
      {
        "type": "input",
        "messageKey": "APIKEY_WEATHERUNDERGROUND",
        "label": "API Key for WeatherUnderground"
      }
    ]
  },
  {
    "type": "section",
    "items": [
      {
        "type": "heading",
        "defaultValue": "Features"
      },
      {
        "type": "toggle",
        "messageKey": "SHOW_ACTIVITY_CURRENT",
        "defaultValue": false,
        "label": "Show current activity",
        "description": "Show current level of activity. Only shown when activity is detected. Having it turned off saves battery."
      },
      {
        "type": "toggle",
        "messageKey": "SHOW_SLEEP",
        "defaultValue": true,
        "label": "Show sleep data"
      },
      {
        "type": "toggle",
        "messageKey": "SHOW_ACTIVITY",
        "defaultValue": true,
        "label": "Show activity data"
      },
      {
        "type": "toggle",
        "messageKey": "SHOW_WEATHER",
        "defaultValue": true,
        "label": "Show weather icon and temperature"
      },
      {
        "type": "toggle",
        "messageKey": "SHOW_PINS_SUN",
        "defaultValue": true,
        "label": "Show pins for sunrise and sunset"
      },
      {
        "type": "toggle",
        "messageKey": "SHOW_PINS_ACHIEVEMENT",
        "defaultValue": true,
        "label": "Show pin when daily average beaten"
      },
      {
        "type": "toggle",
        "messageKey": "SHOW_ACTIVITY_PROGRESS",
        "defaultValue": true,
        "label": "Show current activity vs. avg activity"
      }
    ]
  },
  {
    "type": "submit",
    "defaultValue": "Save"
  }
];