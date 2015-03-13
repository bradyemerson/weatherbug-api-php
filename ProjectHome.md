This is a PHP class that exposes the methods of the WeatherBug API.

**Please note** this is an import of a class I wrote in 2008 and has not been updated to current PHP best practices.  I still make it available because to the best of my knowledge there is no other PHP wrapper publicly available for the WeatherBug API.

If there is renewed interest, I will update it to use more advanced PHP5 methods and would like to make it more compatible with other caching mechanisms.  To show your interest, star the "Update to PHP5" issue on the issues tab.

Feel free to modify it to your purposes (New BSD License).

**Download from the Source tab (/trunk). Downloads tab is empty.**

## Requirements ##
  * PHP 4.3 or higher (including PHP 5)
  * [WeatherBug API key](http://weather.weatherbug.com/desktop-weather/api.html)
  * MySQL if you choose to enable caching
  * [SimplePie](http://simplepie.org/) if you use an RSS function

## MySQL Caching ##
The wrapper supports caching using a mysql database (highly recommended for increased performance). Also, caching provides redudancy of the WeatherBug call fails, the wrapper will fallback to a cached version if it exists. It is enabled by default and you must connect to the database in PHP before calling any methods in the wrapper (using mysql\_connect).

The table structure used:
```
DROP TABLE IF EXISTS `WB_cache`;
CREATE TABLE `WB_cache` ( `request` varchar(35) collate utf8_bin NOT NULL, `response`
 mediumtext collate utf8_bin NOT NULL, `time` int(10) unsigned NOT NULL default '0',
 PRIMARY KEY (`request`) ) ENGINE=MyISAM DEFAULT CHARSET=utf8 COLLATE=utf8_bin;
```

## Basic Usage ##
```
<?php
// Change to path of WBWrapper.inc
require_once 'WBWrapper.inc';

/* Change this to your API Key Also, we disabled caching with the second parameter 
because we have not connected to a database */
$wb = new WBWrapper('APIKEY', false);

if (($forecast = $wb->wb_forecast(array('zipCode'=>90210), 0)) !== false) {        
  foreach($forecast['days'] as $day){
    print_r($day); 
  } 
} else { 
  echo "An Error Occured: {$wb->lastError}"; 
}
```