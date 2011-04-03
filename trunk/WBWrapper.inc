<?php
/*
Software Copyright License Agreement (BSD License)

Copyright (c) 2008 Brady Emerson.  All rights reserved.

Redistribution and use of this software in source and binary forms, with or
without modification, are permitted provided that the following conditions are
met:

* Redistributions of source code must retain the above
  copyright notice, this list of conditions and the following disclaimer.

* Redistributions in binary form must reproduce the above
  copyright notice, this list of conditions and the following disclaimer in the
documentation and/or other materials provided with the distribution.

* Neither my name nor the names of its contributors may be used 
  to endorse or promote products derived from this software without specific prior 
  written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

/**
 * WBWrapper
 *
 * @author Brady Emerson
 * @copyright 2008
 * @version 1.0
 * @access public
 */
class WBWrapper {
	var $wb_key, $cache, $lastError, $lastMySQLError, $lastURL;
	
	// MySQL Cache Table name
	const CACHE_TABLE = 'WB_cache';
	// How long should cache last?
    const CACHE_TIME = 900; // 15 minutes
	
  /**
   * WBWrapper::WBWrapper()
   *
   * @param string $key
   * @param bool $cache
   */
	function WBWrapper($key, $cache=true) {
		$this->wb_key = $key;
		$this->cache = $cache;
	}
	
  /**
   * Returns the location based on the U.S. and international city name or U.S. Zip Code. 
   *
   * @param string|int $searchString
   * @return mixed
   */
	function wb_searchLocations($searchString) {
        if (!$searchString) {
	 	    $this->lastError = "Parameters must be sent in array";
	    	return false;
		}
		$url = "http://".strtolower($this->wb_key).".api.wxbug.net/getLocationsXML.aspx?acode=".strtoupper($this->wb_key)."&SearchString=".urlencode($searchString);
		$this->lastURL = $url;
		$cacheName = md5($url);
		
		/* if we are using caching, first look for a cache response */
		if ($cache = $this->cache_check($cacheName, false)) {
            return $cache;
        }
        
        try {
            $file = file_get_contents($url);
            if (strpos($file, '<?xml version="1.0" encoding="utf-8"?>') === false) {
                throw new Exception('Malformed XML');
            }
            // If we were successful, remove cache
            $this->cache_remove($cacheName);
        } catch (Exception $ex) {
            $this->lastError = $ex->getMessage();
            if ($cache = $this->cache_check($cacheName, true)) {
                return $cache;
            } else {
                return false;
            }
        }
        
        $locations = array();
    	preg_match_all("/<aws:location cityname=\"(.*?)\" statename=\"(.*?)\" countryname=\"(.*?)\" zipcode=\"(.*?)\" citycode=\"(.*?)\" citytype=\"([0-1]{1})\"/", $file, $results);
    	$attributes = array(1=>'cityname', 'statename', 'countryname', 'zipcode', 'citycode', 'citytype');
    	foreach ($results as $key=>$line) {
            if ($key == 0) {
                continue;
            }
            foreach ($line as $id=>$item) {
                $locations[$id][$attributes[$key]] = $item;
            }
        }
        
        // If we are caching, store result
        $this->cache_result($cacheName, $locations);
        // Return array
		return $locations;
    }
	
  /**
   * For a given zipcode, citycode or latitude/longitude this call returns the weather stations in the area. 
   *
   * @param mixed $code
   * @return mixed
   */
	function wb_stationList($code) { 
	 	if (!is_array($code)) {
	 	    $this->lastError = "Parameters must be sent in array";
	    	return false;
		}
		if (array_key_exists('zipCode', $code)) {
            $url = "http://".strtolower($this->wb_key).".api.wxbug.net/getStationsXML.aspx?acode=".strtoupper($this->wb_key)."&zipCode={$code['zipCode']}";
        } else if (array_key_exists('cityCode', $code)) {
            $url = "http://".strtolower($this->wb_key).".api.wxbug.net/getStationsXML.aspx?acode=".strtoupper($this->wb_key)."&cityCode={$code['cityCode']}";
        } else if (array_key_exists('lat', $code) && array_key_exists('long', $code)) {
            $url = "http://".strtolower($this->wb_key).".api.wxbug.net/getStationsXML.aspx?acode=".strtoupper($this->wb_key)."&lat={$code['lat']}&long={$code['long']}";
        } else {
            $this->lastError = "Must send either zipCode, cityCode, or lat/long";
	    	return false;
        }
        $this->lastURL = $url;
		$cacheName = md5($url);
		
		/* if we are using caching, first look for a cache response */
		if ($cache = $this->cache_check($cacheName, false)) {
            return $cache;
        }
        
        try {
            $file = file_get_contents($url);
            if (strpos($file, '<?xml version="1.0" encoding="utf-8"?>') === false) {
                throw new Exception('Malformed XML');
            }
            // If we were successful, remove cache
            $this->cache_remove($cacheName);
        } catch (Exception $ex) {
            $this->lastError = $ex->getMessage();
            if ($cache = $this->cache_check($cacheName, true)) {
                return $cache;
            } else {
                return false;
            }
        }
		
    	$stations = array();
    	preg_match_all("/<aws:station id=\"([A-Z]*)\" name=\"(.*?)\" city=\"(.*?)\" state=\"([A-Z]*?)\" country=\"(.*?)\" zipcode=\"([0-9]*?)\" distance=\"([0-9\.]*?)\" Unit=\"(.*?)\" latitude=\"([0-9\.\-]*?)\" longitude=\"([0-9\.\-]*?)\"/", $file, $results);
    	
    	$attributes = array(1=>'id', 'name', 'city', 'state', 'country', 'zipcode', 'distance', 'unit', 'latitude', 'longitude');
    	foreach ($results as $key=>$line) {
            if ($key == 0) {
                continue;
            }
            foreach ($line as $id=>$item) {
                $stations[$id][$attributes[$key]] = $item;
            }
        }
        
        // If we are caching, store result
        $this->cache_result($cacheName, $stations);
        // Return array
		return $stations;
	}
	
  /**
   * Returns weather alert information for a given Zip Code or latitude/longitude (U.S. Only).
   *
   * @param mixed $code
   * @return mixed
   */
	function wb_alerts($code, $unitType = 0) {
	    if (!is_array($code)) {
	        $this->lastError = "Code must be sent in array";
	    	return false;
		}
		if (array_key_exists('zipCode', $code)) {
            $url = "http://".strtolower($this->wb_key).".api.wxbug.net/getAlertsRSS.aspx?acode=".strtoupper($this->wb_key)."&zipCode={$code['zipCode']}";
        } else if (array_key_exists('lat', $code) && array_key_exists('long', $code)) {
            $url = "http://".strtolower($this->wb_key).".api.wxbug.net/getAlertsRSS.aspx?acode=".strtoupper($this->wb_key)."&lat={$code['lat']}&long={$code['long']}";
        } else {
            $this->lastError = "Must send either zipCode, or lat/long";
	    	return false;
        }
        $url .= "&unitType=$unitType";
        $this->lastURL = $url;
        
		$cacheName = md5($url.'advanced');
				
		/* if we are using caching, first look for a cache response */
		if ($cache = $this->cache_check($cacheName, false)) {
            return $cache;
        }
        
        try {
            $file = file_get_contents($url);
            $file = substr($file, strpos($file, '<aws:weather'));
            $file = substr($file, 0, strpos($file, '</aws:weather>'));
            // If we were successful, remove cache
            $this->cache_remove($cacheName);
        } catch (Exception $ex) {
            $this->lastError = $ex->getMessage();
            if ($cache = $this->cache_check($cacheName, true)) {
                return $cache;
            } else {
                return false;
            }
        }
		
    	$alerts = array();
    	
    	preg_match("/<aws:WebURL>(.*?)<\/aws:WebURL>/", $file, $results);
    	$alerts['webURL'] = $results[1];
    	$alerts['alerts'] = array();
    	
    	preg_match_all("/<aws:alert><aws:id>(.*?)<\/aws:id><aws:type>(.*?)<\/aws:type><aws:title>(.*?)<\/aws:title><aws:posted-date><aws:year number=\"([0-9]*?)\" \/><aws:month number=\"([0-9]*?)\" text=\"(\w*?)\" abbrv=\"(\w*?)\" \/><aws:day number=\"(.*?)\" text=\"(\w*?)\" abbrv=\"(\w*?)\" \/><aws:hour number=\"([0-9]*?)\" hour-24=\"([0-9]*?)\" \/><aws:minute number=\"([0-9]*?)\" \/><aws:second number=\"([0-9]*?)\" \/><aws:am-pm abbrv=\"(\w*?)\" \/><aws:time-zone offset=\"([0-9\-]*?)\" text=\"(.*?)\" abbrv=\"(\w*?)\" \/><\/aws:posted-date><aws:expires-date><aws:year number=\"([0-9]*?)\" \/><aws:month number=\"([0-9]*?)\" text=\"(\w*?)\" abbrv=\"(\w*?)\" \/><aws:day number=\"(.*?)\" text=\"(\w*?)\" abbrv=\"(\w*?)\" \/><aws:hour number=\"([0-9]*?)\" hour-24=\"([0-9]*?)\" \/><aws:minute number=\"([0-9]*?)\" \/><aws:second number=\"([0-9]*?)\" \/><aws:am-pm abbrv=\"(\w*?)\" \/><aws:time-zone offset=\"([0-9\-]*?)\" text=\"(.*?)\" abbrv=\"(\w*?)\" \/><\/aws:expires-date><aws:msg-summary>(.*?)<\/aws:msg-summary><\/aws:alert>/", $file, $results);
    	
    	$attributes = array(1=>'id', 'type', 'title', 'year', 'number', 'text', 'abbrv', 'number', 'text', 'abbrv', '12', '24', 'minute', 'second', 'am-pm', 'offset', 'text', 'abbrv', 'year', 'number', 'text', 'abbrv', 'number', 'text', 'abbrv', '12', '24', 'minute', 'second', 'am-pm', 'offset', 'text', 'abbrv', 'msg-summary');
    	
    	foreach ($results as $key=>$line) {
            if ($key == 0) {
                continue;
            }
            foreach ($line as $id=>$item) {
                if ($key >=4 && $key <= 18) {
                    if ($key >= 5 && $key <= 7) {
                        $alerts['alerts'][$id]['posted-date']['month'][$attributes[$key]] = $item;
                        $alerts['alerts'][$id]['posted-date']['month'][$attributes[$key]] = $item;
                        $alerts['alerts'][$id]['posted-date']['month'][$attributes[$key]] = $item;
                    } else if ($key >= 8 && $key <= 10) {
                        $alerts['alerts'][$id]['posted-date']['day'][$attributes[$key]] = $item;
                		$alerts['alerts'][$id]['posted-date']['day'][$attributes[$key]] = $item;
                		$alerts['alerts'][$id]['posted-date']['day'][$attributes[$key]] = $item;
                    } else if ($key == 11 || $key == 12) {
                        $alerts['alerts'][$id]['posted-date']['hour'][$attributes[$key]] = $item;
                        $alerts['alerts'][$id]['posted-date']['hour'][$attributes[$key]] = $item;
                    } else if ($key >= 16 && $key <= 18) {
                        $alerts['alerts'][$id]['posted-date']['timezone'][$attributes[$key]] = $item;
                        $alerts['alerts'][$id]['posted-date']['timezone'][$attributes[$key]] = $item;
                        $alerts['alerts'][$id]['posted-date']['timezone'][$attributes[$key]] = $item;
                    } else {
                        $alerts['alerts'][$id]['posted-date'][$attributes[$key]] = $item;
                    }
                } else if ($key >= 19 && $key <= 33) {
                    if ($key >= 20 && $key <= 22) {
                        $alerts['alerts'][$id]['expires-date']['month'][$attributes[$key]] = $item;
                        $alerts['alerts'][$id]['expires-date']['month'][$attributes[$key]] = $item;
                        $alerts['alerts'][$id]['expires-date']['month'][$attributes[$key]] = $item;
                    } else if ($key >= 23 && $key <= 25) {
                        $alerts['alerts'][$id]['expires-date']['day'][$attributes[$key]] = $item;
                		$alerts['alerts'][$id]['expires-date']['day'][$attributes[$key]] = $item;
                		$alerts['alerts'][$id]['expires-date']['day'][$attributes[$key]] = $item;
                    } else if ($key == 26 || $key == 27) {
                        $alerts['alerts'][$id]['expires-date']['hour'][$attributes[$key]] = $item;
                        $alerts['alerts'][$id]['expires-date']['hour'][$attributes[$key]] = $item;
                    } else if ($key >= 31 && $key <= 33) {
                        $alerts['alerts'][$id]['expires-date']['timezone'][$attributes[$key]] = $item;
                        $alerts['alerts'][$id]['expires-date']['timezone'][$attributes[$key]] = $item;
                        $alerts['alerts'][$id]['expires-date']['timezone'][$attributes[$key]] = $item;
                    } else {
                        $alerts['alerts'][$id]['expires-date'][$attributes[$key]] = $item;
                    }
                } else {
                    $alerts['alerts'][$id][$attributes[$key]] = $item;
                }
                
            }
        }
        
        // If we are caching, store result
        $this->cache_result($cacheName, $alerts);
        // Return array
		return $alerts;
	}
	
  /**
   * Returns weather alert information for a given Zip Code or latitude/longitude (U.S. Only).
   *
   * @param mixed $code
   * @param integer $unitType
   * @return mixed
   */
	function wb_alertsRSS($code, $unitType = 0) {
	    require_once('simplepie.inc');
        include_once('idn/idna_convert.class.php');
        
        if (!is_array($code)) {
	        $this->lastError = "Code must be sent in array";
	    	return false;
		}
		if (array_key_exists('zipCode', $code)) {
            $url = "http://".strtolower($this->wb_key).".api.wxbug.net/getAlertsRSS.aspx?acode=".strtoupper($this->wb_key)."&zipCode={$code['zipCode']}";
        } else if (array_key_exists('lat', $code) && array_key_exists('long', $code)) {
            $url = "http://".strtolower($this->wb_key).".api.wxbug.net/getAlertsRSS.aspx?acode=".strtoupper($this->wb_key)."&lat={$code['lat']}&long={$code['long']}";
        } else {
            $this->lastError = "Must send either zipCode, or lat/long";
	    	return false;
        }
        $url .= "&unitType=$unitType";
        $this->lastURL = $url;
		$cacheName = md5($url);
		
		/* if we are using caching, first look for a cache response */
		if ($cache = $this->cache_check($cacheName, false)) {
            return $cache;
        }
        
        $feed = new SimplePie();
        $feed->set_feed_url($url);
        $feed->enable_cache(false);
        // Increase speed
		$feed->remove_div(false);
		$feed->set_image_handler(false);
		$feed->set_autodiscovery_level(SIMPLEPIE_LOCATOR_NONE);
		$feed->init();
		
		if ($feed->data) {
            // If we were successful, remove cache
            $this->cache_remove($cacheName);
            $alerts = array();
            $alerts['title'] = $feed->get_title();
            $alerts['image'] = $feed->get_image_url();
            $alerts['link'] = $feed->get_link();
            
            foreach ($feed->get_items() as $item) {
                $day = array('title'=>$item->get_title(), 'description'=>$item->get_description(), 'timestamp'=>$item->get_date('U'), 'link'=>$item->get_link());
                $alerts['alerts'][] = $day;
            }
            
            // If we are caching, store result
            $this->cache_result($cacheName, $alerts);
            // Return array
    		return $alerts;
    		
        } else {
            $this->lastError = $feed->error();
            if ($cache = $this->cache_check($cacheName, true)) {
                return $cache;
            } else {
                return false;
            }
        }
    }
	
		
  /**
   * Returns the 7-day forecast for a zipcode, citycode or latitude/longitude. 
   *
   * @param mixed $code
   * @param integer $unitType
   * @return mixed
   */
	function wb_forecast($code, $unitType = 0) {
	    if (!is_array($code)) {
	        $this->lastError = "Parameters must be sent in array";
	    	return false;
		}
		if (array_key_exists('zipCode', $code)) {
            $url = "http://".strtolower($this->wb_key).".api.wxbug.net/getForecastRSS.aspx?acode=".strtoupper($this->wb_key)."&zipCode={$code['zipCode']}";
        } else if (array_key_exists('cityCode', $code)) {
            $url = "http://".strtolower($this->wb_key).".api.wxbug.net/getForecastRSS.aspx?acode=".strtoupper($this->wb_key)."&cityCode={$code['cityCode']}";
        } else if (array_key_exists('lat', $code) && array_key_exists('long', $code)) {
            $url = "http://".strtolower($this->wb_key).".api.wxbug.net/getForecastRSS.aspx?acode=".strtoupper($this->wb_key)."&lat={$code['lat']}&long={$code['long']}";
        } else {
            $this->lastError = "Must send either zipCode, cityCode, or lat/long";
	    	return false;
        }
		$url .= "&unitType=$unitType";
		$this->lastURL = $url;
		$cacheName = md5($url.'advanced');
				
		/* if we are using caching, first look for a cache response */
		if ($cache = $this->cache_check($cacheName, false)) {
            return $cache;
        }
        
        try {
            $file = file_get_contents($url);
            $file = substr($file, strpos($file, '<aws:weather'));
            $file = substr($file, 0, strpos($file, '</aws:weather>'));
            // If we were successful, remove cache
            $this->cache_remove($cacheName);
        } catch (Exception $ex) {
            $this->lastError = $ex->getMessage();
            if ($cache = $this->cache_check($cacheName, true)) {
                return $cache;
            } else {
                return false;
            }
        }
		
    	$forecast = array();
    	// Add up generic forecast information
    	preg_match("/<aws:WebURL>(.*?)<\/aws:WebURL><aws:forecasts type=\"(.*?)\" date=\"(.*?)\"><aws:location><aws:city>(.*?)<\/aws:city><aws:state>(.*?)<\/aws:state><aws:zip>([0-9]*?)<\/aws:zip><aws:zone>(.*?)<\/aws:zone><\/aws:location>/", $file, $results);
    	
    	$forecast['webURL'] = $results[1];
    	$forecast['type'] = $results[2];
        $forecast['date'] = $results[3];
		$forecast['location']['city'] = $results[4];
		$forecast['location']['state'] = $results[5];
		$forecast['location']['zipcode'] = $results[6];
		$forecast['location']['zone'] = $results[7];
		
		// Now add each forecast	
    	preg_match_all("/<aws:forecast><aws:title alttitle=\"(.*?)\">(.*?)<\/aws:title><aws:short-prediction>(.*?)<\/aws:short-prediction><aws:image isNight=\"([0-1]{1})\" icon=\"(.*?)\">(.*?)<\/aws:image><aws:description>(.*?)<\/aws:description><aws:prediction>(.*?)<\/aws:prediction><aws:high units=\"(.*?)\">(.*?)<\/aws:high><aws:low units=\"(.*?)\">(.*?)<\/aws:low>/", $file, $results);
    	
    	$attributes = array(1=>'short-title', 'title', 'short-prediction', 'isNight', 'img', 'url', 'description', 'prediction', 'unit', 'value', 'unit', 'value');
    	foreach ($results as $key=>$line) {
            if ($key == 0) {
                continue;
            }
            foreach ($line as $id=>$item) {
                if ($key == 4 || $key == 5 || $key == 6) {
                    $forecast['days'][$id]['icon'][$attributes[$key]] = $item;
                } else if ($key == 9 || $key == 10) {
                    $forecast['days'][$id]['high'][$attributes[$key]] = ($key == 9?str_replace('&amp;', '&', $item):$item);
                } else if ($key == 11 || $key == 12) {
                    $forecast['days'][$id]['low'][$attributes[$key]] = ($key == 11?str_replace('&amp;', '&', $item):$item);
                } else {
                    $forecast['days'][$id][$attributes[$key]] = $item;
                }
            }
        }
        
        // If we are caching, store result
        $this->cache_result($cacheName, $forecast);
        // Return array
		return $forecast;
	}
	
  /**
   * Returns the 7-day forecast for a zipcode, citycode or latitude/longitude. 
   *
   * @param mixed $code
   * @param integer $unitType
   * @return mixed
   */
	function wb_forecastRSS($code, $unitType = 0) {
	    require_once('simplepie.inc');
        include_once('idn/idna_convert.class.php');
        
        if (!is_array($code)) {
	 	    $this->lastError = "Code must be sent in array";
	    	return false;
		}
		if (array_key_exists('zipCode', $code)) {
            $url = "http://".strtolower($this->wb_key).".api.wxbug.net/getForecastRSS.aspx?acode=".strtoupper($this->wb_key)."&zipCode={$code['zipCode']}";
        } else if (array_key_exists('cityCode', $code)) {
            $url = "http://".strtolower($this->wb_key).".api.wxbug.net/getForecastRSS.aspx?acode=".strtoupper($this->wb_key)."&cityCode={$code['cityCode']}";
        } else if (array_key_exists('lat', $code) && array_key_exists('long', $code)) {
            $url = "http://".strtolower($this->wb_key).".api.wxbug.net/getForecastRSS.aspx?acode=".strtoupper($this->wb_key)."&lat={$code['lat']}&long={$code['long']}";
        } else {
            $this->lastError = "Must send either zipCode, cityCode, or lat/long";
	    	return false;
        }
        $url .= "&unitType=$unitType";
        $this->lastURL = $url;
		$cacheName = md5($url);
		
		/* if we are using caching, first look for a cache response */
		if ($cache = $this->cache_check($cacheName, false)) {
            return $cache;
        }
        
        $feed = new SimplePie();
        $feed->set_feed_url($url);
        $feed->enable_cache(false);
        // Increase speed
		$feed->remove_div(false);
		$feed->set_image_handler(false);
		$feed->set_autodiscovery_level(SIMPLEPIE_LOCATOR_NONE);
		$feed->init();
		
		if ($feed->data) {
            // If we were successful, remove cache
            $this->cache_remove($cacheName);
            $forecast = array();
            $forecast['title'] = $feed->get_title();
            $forecast['image'] = $feed->get_image_url();
            $forecast['link'] = $feed->get_link();
            
            foreach ($feed->get_items() as $item) {
                $day = array('title'=>$item->get_title(), 'description'=>$item->get_description(), 'timestamp'=>$item->get_date('U'), 'link'=>$item->get_link());
                $forecast['days'][] = $day;
            }
            
            // If we are caching, store result
            $this->cache_result($cacheName, $forecast);
            // Return array
    		return $forecast;
    		
        } else {
            $this->lastError = $feed->error();
            if ($cache = $this->cache_check($cacheName, true)) {
                return $cache;
            } else {
                return false;
            }
        }
    }
    
    
  /**
   * WBWrapper::wb_liveWeather()
   *
   * @param mixed $code
   * @param integer $unitType
   * @return mixed
   */
    function wb_liveWeather($code, $unitType = 0) {
        require_once('simplepie.inc');
        include_once('idn/idna_convert.class.php');
        
        if (!is_array($code)) {
	 	    $this->lastError = "Parameters must be sent in array";
	    	return false;
		}
		if (array_key_exists('stationId', $code)) {
		    if (is_array($code['stationId'])) {
                $code['stationId'] = implode(',',$code['stationId']);
            }
            $url = "http://".strtolower($this->wb_key).".api.wxbug.net/getLiveWeatherRSS.aspx?acode=".strtoupper($this->wb_key)."&stationid={$code['stationId']}";
		} else if (array_key_exists('zipCode', $code)) {
            $url = "http://".strtolower($this->wb_key).".api.wxbug.net/getLiveWeatherRSS.aspx?acode=".strtoupper($this->wb_key)."&zipCode={$code['zipCode']}";
        } else if (array_key_exists('cityCode', $code)) {
            $url = "http://".strtolower($this->wb_key).".api.wxbug.net/getLiveWeatherRSS.aspx?acode=".strtoupper($this->wb_key)."&cityCode={$code['cityCode']}";
        } else if (array_key_exists('lat', $code) && array_key_exists('long', $code)) {
            $url = "http://".strtolower($this->wb_key).".api.wxbug.net/getLiveWeatherRSS.aspx?acode=".strtoupper($this->wb_key)."&lat={$code['lat']}&long={$code['long']}";
        } else {
            $this->lastError = "Must send either stationId, zipCode, cityCode, or lat/long";
	    	return false;
        }
        $this->lastURL = $url;
		$cacheName = md5($url);
		
		/* if we are using caching, first look for a cache response */
		if ($cache = $this->cache_check($cacheName, false)) {
            return $cache;
        }
        
        $feed = new SimplePie();
        $feed->set_feed_url($url);
        $feed->enable_cache(false);
        // Increase speed
		$feed->remove_div(false);
		$feed->set_image_handler(false);
		$feed->set_autodiscovery_level(SIMPLEPIE_LOCATOR_NONE);
		$feed->init();
		
		if ($feed->data) {
            // If we were successful, remove cache
            $this->cache_remove($cacheName);
            $weather = array();
            $weather['title'] = $feed->get_title();
            $weather['image'] = $feed->get_image_url();
            $weather['link'] = $feed->get_link();
            
            foreach ($feed->get_items() as $item) {
                $day = array('title'=>$item->get_title(), 'description'=>$item->get_description(), 'timestamp'=>$item->get_date('U'), 'link'=>$item->get_link());
                $weather['weather'][] = $day;
            }
            
            // If we are caching, store result
            $this->cache_result($cacheName, $weather);
            // Return array
    		return $weather;
    		
        } else {
            $this->lastError = $feed->error();
            if ($cache = $this->cache_check($cacheName, true)) {
                return $cache;
            } else {
                return false;
            }
        }
    }
    
  /**
   * WBWrapper::wb_compactLiveWeather()
   *
   * @param mixed $code
   * @param integer $unitType
   * @return mixed
   */
    function wb_compactLiveWeather($code, $unitType = 0) {
        require_once('simplepie.inc');
        include_once('idn/idna_convert.class.php');
        
        if (!is_array($code)) {
	 	    $this->lastError = "Parameters must be sent in array";
	    	return false;
		}
		if (array_key_exists('stationId', $code)) {
		    if (is_array($code['stationId'])) {
                $code['stationId'] = implode(',',$code['stationId']);
            }
            $url = "http://".strtolower($this->wb_key).".api.wxbug.net/getLiveCompactWeatherRSS.aspx?acode=".strtoupper($this->wb_key)."&stationid={$code['stationId']}";
		} else if (array_key_exists('zipCode', $code)) {
            $url = "http://".strtolower($this->wb_key).".api.wxbug.net/getLiveCompactWeatherRSS.aspx?acode=".strtoupper($this->wb_key)."&zipCode={$code['zipCode']}";
        } else if (array_key_exists('cityCode', $code)) {
            $url = "http://".strtolower($this->wb_key).".api.wxbug.net/getLiveCompactWeatherRSS.aspx?acode=".strtoupper($this->wb_key)."&cityCode={$code['cityCode']}";
        } else if (array_key_exists('lat', $code) && array_key_exists('long', $code)) {
            $url = "http://".strtolower($this->wb_key).".api.wxbug.net/getLiveCompactWeatherRSS.aspx?acode=".strtoupper($this->wb_key)."&lat={$code['lat']}&long={$code['long']}";
        } else {
            $this->lastError = "Must send either stationId, zipCode, cityCode, or lat/long";
	    	return false;
        }
        $this->lastURL = $url;
		$cacheName = md5($url);
		
		/* if we are using caching, first look for a cache response */
		if ($cache = $this->cache_check($cacheName, false)) {
            return $cache;
        }
        
        $feed = new SimplePie();
        $feed->set_feed_url($url);
        $feed->enable_cache(false);
        // Increase speed
		$feed->remove_div(false);
		$feed->set_image_handler(false);
		$feed->set_autodiscovery_level(SIMPLEPIE_LOCATOR_NONE);
		$feed->init();
		
		if ($feed->data) {
            // If we were successful, remove cache
            $this->cache_remove($cacheName);
            $weather = array();
            $weather['title'] = $feed->get_title();
            $weather['image'] = $feed->get_image_url();
            $weather['link'] = $feed->get_link();
            
            foreach ($feed->get_items() as $item) {
                $day = array('title'=>$item->get_title(), 'description'=>$item->get_description(), 'timestamp'=>$item->get_date('U'), 'link'=>$item->get_link());
                $weather['weather'][] = $day;
            }
            
            // If we are caching, store result
            $this->cache_result($cacheName, $weather);
            // Return array
    		return $weather;
    		
        } else {
            $this->lastError = $feed->error();
            if ($cache = $this->cache_check($cacheName, true)) {
                return $cache;
            } else {
                return false;
            }
        }
    }
	
	
  /**
   * *Legacy Function* - Returns concise live weather conditions for the chosen location.
   *
   * @param string $stat_id
   * @param integer $unitType
   * @return mixed
   */
	function wb_compact($stat_id, $unitType = 0) {
	    if (!$stat_id) {
	 	    $this->lastError = "No Station ID Passed";
	    	return false;
		}
		
		$url = "http://".strtolower($this->wb_key).".api.wxbug.net/getLiveCompactWeather.aspx?acode=".strtoupper($this->wb_key)."&StationID=$stat_id&unitType=$unitType";
		$this->lastURL = $url;
		$cacheName = md5($url);
				
		/* if we are using caching, first look for a cache response */
		if ($cache = $this->cache_check($cacheName, false)) {
            return $cache;
        }
        
        try {
            $file = file_get_contents($url);
            if (strpos($file, '<?xml version="1.0" encoding="utf-8"?>') === false) {
                throw new Exception('Malformed XML');
            }
            // If we were successful, remove cache
            $this->cache_remove($cacheName);
        } catch (Exception $ex) {
            $this->lastError = $ex->getMessage();
            if ($cache = $this->cache_check($cacheName, true)) {
                return $cache;
            } else {
                return false;
            }
        }
        
    	preg_match("/<aws:station id=\"([A-Z]*?)\" name=\"(.*?)\" city=\"(.*?)\" state=\"(.*?)\" zipcode=\"([0-9]*?)\" \/><aws:temp units=\"(.*?)\">(.*?)<\/aws:temp><aws:rain-today units=\"(.*?)\">(.*?)<\/aws:rain-today><aws:wind-speed units=\"(.*?)\">([0-9]*?)<\/aws:wind-speed><aws:wind-direction>(.*?)<\/aws:wind-direction><aws:gust-speed units=\"(.*?)\">([0-9]*?)<\/aws:gust-speed><aws:gust-direction>(.*?)<\/aws:gust-direction>/", $file, $result);
    	
		$station = array();
		// Station Info
		$station['station'] = array();
		$station['station']['id'] = $result[1];
		$station['station']['name'] = $result[2];
		$station['station']['city'] = $result[3];
		$station['station']['state'] = trim($result[4]);
		$station['station']['zipcode'] = $result[5];
		// Temp
		$station['temp'] = array();
		$station['temp']['value'] = str_replace('&amp;', '&', $result[7]);
		$station['temp']['unit'] = $result[6];
		// rain
		$station['rain'] = array();
		$station['rain']['value'] = $result[9];
		$station['rain']['unit'] = $result[8];
		// Wind
		$station['wind'] = array();
		$station['wind']['value'] = $result[11];
		$station['wind']['unit'] = $result[10];
		$station['wind']['direction'] = $result[12];
		// Gusts
		$station['gust'] = array();
		$station['gust']['value'] = $result[14];
		$station['gust']['unit'] = $result[13];
		$station['gust']['direction'] = $result[15];
		
		// If we are caching, store result
        $this->cache_result($cacheName, $station);
        // Return array
		return $station;
	}
	
  /**
   * Returns detailed live weather conditions for the chosen location. 
   *
   * @param string $stat_id
   * @param integer $unitType
   * @return mixed
   */
	function wb_complete ($stat_id, $unitType = 0) {
		if (!$stat_id) {
	 	    $this->lastError = "No Station ID Passed";
	    	return false;
		}
		
		$url = "http://".strtolower($this->wb_key).".api.wxbug.net/getLiveWeather.aspx?acode=".strtoupper($this->wb_key)."&StationID=$stat_id&unitType=$unitType";
		$this->lastURL = $url;
		$cacheName = md5($url);
				
		/* if we are using caching, first look for a cache response */
		if ($cache = $this->cache_check($cacheName, false)) {
            return $cache;
        }
        
        try {
            $file = file_get_contents($url);
            if (strpos($file, '<?xml version="1.0" encoding="utf-8"?>') === false) {
                throw new Exception('Malformed XML');
            }
            // If we were successful, remove cache
            $this->cache_remove($cacheName);
        } catch (Exception $ex) {
            $this->lastError = $ex->getMessage();
            if ($cache = $this->cache_check($cacheName, true)) {
                return $cache;
            } else {
                return false;
            }
        }
		$station = array();
		// date
		preg_match("/<aws:year number=\"([0-9]*?)\" \/><aws:month number=\"([0-9]*?)\" text=\"(\w*?)\" abbrv=\"(\w*?)\" \/><aws:day number=\"(.*?)\" text=\"(\w*?)\" abbrv=\"(\w*?)\" \/><aws:hour number=\"([0-9]*?)\" hour-24=\"([0-9]*?)\" \/><aws:minute number=\"([0-9]*?)\" \/><aws:second number=\"([0-9]*?)\" \/><aws:am-pm abbrv=\"(\w*?)\" \/><aws:time-zone offset=\"([0-9\-]*?)\" text=\"(.*?)\" abbrv=\"(\w*?)\" \/><\/aws:ob\-date>/", $file, $result);
		$station['date']['year'] = $result[1];
		$station['date']['month']['number'] = $result[2];
		$station['date']['month']['text'] = $result[3];
		$station['date']['month']['abbrv'] = $result[4];
		$station['date']['day']['number'] = $result[5];
		$station['date']['day']['text'] = $result[6];
		$station['date']['day']['abbrv'] = $result[7];
		$station['date']['hour']['12'] = $result[8];
		$station['date']['hour']['24'] = $result[9];
		$station['date']['minute'] = $result[10];
		$station['date']['second'] = $result[11];
		$station['date']['am-pm'] = $result[12];
		$station['date']['timezone']['offset'] = $result[13];
		$station['date']['timezone']['text'] = $result[14];
		$station['date']['timezone']['abbrv'] = $result[15];
		
		// station
		preg_match("/<aws:station-id>([A-Z]*?)<\/aws:station-id><aws:station>(.*?)<\/aws:station><aws:city-state zip=\"([0-9]*?)\">(.*?), (.*?)<\/aws:city-state>/", $file, $result);
		$station['station']['id'] = $result[1];
		$station['station']['name'] = $result[2];
		$station['station']['zipcode'] = $result[3];
		$station['station']['city'] = $result[4];
		$station['station']['state'] = trim($result[5]);
		/**
		* URL is a little more tricky cause it is not uniform, if there is one it is like this:
		* <aws:site-url>url</aws:site-url>, if there isn't one its like this: <aws:site-url />
		*/
		if (preg_match("/<aws:site-url>(.*?)<\/aws:site-url>/", $file, $result)) {
			$station['station']['url'] = $result[1];
		} else {
			$station['station']['url'] = '';	
		}
		// station - elevation
		preg_match("/<aws:elevation units=\"(.*?)\">([0-9]*?)<\/aws:elevation>/", $file, $result);
		$station['station']['elevation']['units'] = $result[1];
		$station['station']['elevation']['value'] = $result[2];
		
		// dew-point
		preg_match("/<aws:dew-point units=\"(.*?)\">(.*?)<\/aws:dew-point>/", $file, $result);
		$station['dew-point']['units'] = str_replace('&amp;', '&', $result[1]);
		$station['dew-point']['value'] = floatval($result[2]);
		
		// Gust - Time and Value
		preg_match("/<aws:gust-time xmlns:aws=\"http:\/\/www.aws.com\/aws\"><aws:year number=\"([0-9]*?)\" \/><aws:month number=\"([0-9]*?)\" text=\"(\w*?)\" abbrv=\"(\w*?)\" \/><aws:day number=\"(.*?)\" text=\"(\w*?)\" abbrv=\"(\w*?)\" \/><aws:hour number=\"([0-9]*?)\" hour-24=\"([0-9]*?)\" \/><aws:minute number=\"([0-9]*?)\" \/><aws:second number=\"([0-9]*?)\" \/><aws:am-pm abbrv=\"(\w*?)\" \/><aws:time-zone offset=\"([0-9\-]*?)\" text=\"(.*?)\" abbrv=\"(\w*?)\" \/><\/aws:gust-time><aws:gust-direction>(\w*?)<\/aws:gust-direction><aws:gust-speed units=\"(.*?)\">([0-9\.]*?)<\/aws:gust-speed>/", $file, $result);
		$station['gust']['year'] = $result[1];
		$station['gust']['month']['number'] = $result[2];
		$station['gust']['month']['text'] = $result[3];
		$station['gust']['month']['abbrv'] = $result[4];
		$station['gust']['day']['number'] = $result[5];
		$station['gust']['day']['text'] = $result[6];
		$station['gust']['day']['abbrv'] = $result[7];
		$station['gust']['hour']['12'] = $result[8];
		$station['gust']['hour']['24'] = $result[9];
		$station['gust']['minute'] = $result[10];
		$station['gust']['second'] = $result[11];
		$station['gust']['am-pm'] = $result[12];
		$station['gust']['timezone']['offset'] = $result[13];
		$station['gust']['timezone']['text'] = $result[14];
		$station['gust']['timezone']['abbrv'] = $result[15];
		
		$station['gust']['direction'] = $result[16];
		$station['gust']['units'] = $result[17];
		$station['gust']['value'] = $result[18];
		
		// humidity
		preg_match("/<aws:humidity units=\"(.*?)\">([0-9\.]*?)<\/aws:humidity><aws:humidity-high units=\"(.*?)\">(.*?)<\/aws:humidity-high><aws:humidity-low units=\"(.*?)\">([0-9\.]*?)<\/aws:humidity-low><aws:humidity-rate>([0-9\.\-+]*?)<\/aws:humidity-rate>/", $file, $result);
		$station['humidity']['units'] = $result[1];
		$station['humidity']['value'] = $result[2];
		$station['humidity']['high']['units'] = $result[3];
		$station['humidity']['high']['value'] = $result[4];
		$station['humidity']['low']['units'] = $result[5];
		$station['humidity']['low']['value'] = $result[6];
		$station['humidity']['rate'] = $result[7];
		
		// indoor
		preg_match("/<aws:indoor-temp units=\"(.*?)\">(.*?)<\/aws:indoor-temp><aws:indoor-temp-rate units=\"(.*?)\">(.*?)<\/aws:indoor-temp-rate>/", $file, $result);
		$station['indoor']['units'] = str_replace('&amp;', '&', $result[1]);
		$station['indoor']['value'] = floatval($result[2]);
		$station['indoor']['rate']['units'] = str_replace('&amp;', '&', $result[3]);
		$station['indoor']['rate']['value'] = floatval($result[4]);
		
		// light
		preg_match("/<aws:light>([0-9\.]*?)<\/aws:light><aws:light-rate>(.*?)<\/aws:light-rate>/", $file, $result);
		$station['light']['value'] = $result[1];
		$station['light']['rate'] = $result[2];
		
		// moon
		preg_match("/<aws:moon-phase moon-phase-img=\"(.*?)\">(.*?)<\/aws:moon-phase>/", $file, $result);
		$station['moon']['value'] = $result[2];
		$station['moon']['img'] = $result[1];
		
		// pressure
		preg_match("/<aws:pressure units=\"(.*?)\">([0-9\-\.]*?)<\/aws:pressure><aws:pressure-high units=\"(.*?)\">(.*)<\/aws:pressure-high><aws:pressure-low units=\"(.*?)\">([0-9\-\.]*?)<\/aws:pressure-low><aws:pressure-rate units=\"(.*?)\">([0-9\-\.]*?)<\/aws:pressure-rate>/", $file, $result);
		$station['pressure']['units'] = $result[1];
		$station['pressure']['value'] = $result[2];
		$station['pressure']['high']['units'] = $result[3];
		$station['pressure']['high']['value'] = $result[4];
		$station['pressure']['low']['units'] = $result[5];
		$station['pressure']['low']['value'] = $result[6];
		$station['pressure']['rate']['units'] = $result[7];
		$station['pressure']['rate']['value'] = $result[8];
		
		// rain
		preg_match("/<aws:rain-month units=\"(.*)\">([0-9\.]*?)<\/aws:rain-month><aws:rain-rate units=\"(.*)\">([0-9\.]*?)<\/aws:rain-rate><aws:rain-rate-max units=\"(.*)\">([0-9\.]*?)<\/aws:rain-rate-max><aws:rain-today units=\"(.*)\">([0-9\.]*?)<\/aws:rain-today><aws:rain-year units=\"(.*)\">([0-9\.]*?)<\/aws:rain-year>/", $file, $result);
		$station['rain']['month']['units'] = $result[1];
		$station['rain']['month']['value'] = $result[2];
		$station['rain']['rate']['units'] = $result[3];
		$station['rain']['rate']['value'] = $result[4];
		$station['rain']['rate']['max']['units'] = $result[5];
		$station['rain']['rate']['max']['value'] = $result[6];
		$station['rain']['today']['units'] = $result[7];
		$station['rain']['today']['value'] = $result[8];
		$station['rain']['year']['units'] = $result[9];
		$station['rain']['year']['value'] = $result[10];
		
		// temp
		preg_match("/<aws:temp units=\"(.*?)\">(.*?)<\/aws:temp><aws:temp-high units=\"(.*?)\">(.*?)<\/aws:temp-high><aws:temp-low units=\"(.*?)\">(.*?)<\/aws:temp-low><aws:temp-rate units=\"(.*?)\">(.*?)<\/aws:temp-rate>/", $file, $result);
		$station['temp']['units'] = str_replace('&amp;', '&', $result[1]);
		$station['temp']['value'] = floatval($result[2]);
		$station['temp']['high']['units'] = str_replace('&amp;', '&', $result[3]);
		$station['temp']['high']['value'] = floatval($result[4]);
		$station['temp']['low']['units'] = str_replace('&amp;', '&', $result[5]);
		$station['temp']['low']['value'] = floatval($result[6]);
		$station['temp']['rate']['units'] = str_replace('&amp;', '&', $result[7]);
		$station['temp']['rate']['value'] = floatval($result[8]);
		//temp - feels like
		preg_match("/<aws:feels-like units=\"(.*?)\">(.*?)<\/aws:feels-like>/", $file, $result);
		$station['temp']['feels-like']['units'] = str_replace('&amp;', '&', $result[1]);
		$station['temp']['feels-like']['value'] = floatval($result[2]);
		//temp - aux
		preg_match("/<aws:aux-temp units=\"(.*?)\">([0-9\.\-]*?)<\/aws:aux-temp><aws:aux-temp-rate units=\"(.*?)\">(.*?)<\/aws:aux-temp-rate>/", $file, $result);
		$station['temp']['aux']['units'] = str_replace('&amp;', '&', $result[1]);
		$station['temp']['aux']['value'] = $result[2];
		$station['temp']['aux']['rate']['units'] = str_replace('&amp;', '&', $result[3]);
		$station['temp']['aux']['rate']['value'] = $result[4];
		
		// sunrise
		preg_match("/<aws:sunrise xmlns:aws=\"http:\/\/www.aws.com\/aws\"><aws:year number=\"([0-9]*?)\" \/><aws:month number=\"([0-9]*?)\" text=\"(\w*?)\" abbrv=\"(\w*?)\" \/><aws:day number=\"(.*?)\" text=\"(\w*?)\" abbrv=\"(\w*?)\" \/><aws:hour number=\"([0-9]*?)\" hour-24=\"([0-9]*?)\" \/><aws:minute number=\"([0-9]*?)\" \/><aws:second number=\"([0-9]*?)\" \/><aws:am-pm abbrv=\"(\w*?)\" \/><\/aws:sunrise>/", $file, $result);
		$station['sunrise']['year'] = $result[1];
		$station['sunrise']['month']['number'] = $result[2];
		$station['sunrise']['month']['text'] = $result[3];
		$station['sunrise']['month']['abbrv'] = $result[4];
		$station['sunrise']['day']['number'] = $result[5];
		$station['sunrise']['day']['text'] = $result[6];
		$station['sunrise']['day']['abbrv'] = $result[7];
		$station['sunrise']['hour']['12'] = $result[8];
		$station['sunrise']['hour']['24'] = $result[9];
		$station['sunrise']['minute'] = $result[10];
		$station['sunrise']['second'] = $result[11];
		$station['sunrise']['am-pm'] = $result[12];
		
		// sunset
		preg_match("/<aws:sunset xmlns:aws=\"http:\/\/www.aws.com\/aws\"><aws:year number=\"([0-9]*?)\" \/><aws:month number=\"([0-9]*?)\" text=\"(\w*?)\" abbrv=\"(\w*?)\" \/><aws:day number=\"(.*?)\" text=\"(\w*?)\" abbrv=\"(\w*?)\" \/><aws:hour number=\"([0-9]*?)\" hour-24=\"([0-9]*?)\" \/><aws:minute number=\"([0-9]*?)\" \/><aws:second number=\"([0-9]*?)\" \/><aws:am-pm abbrv=\"(\w*?)\" \/><\/aws:sunset>/", $file, $result);
		$station['sunset']['year'] = $result[1];
		$station['sunset']['month']['number'] = $result[2];
		$station['sunset']['month']['text'] = $result[3];
		$station['sunset']['month']['abbrv'] = $result[4];
		$station['sunset']['day']['number'] = $result[5];
		$station['sunset']['day']['text'] = $result[6];
		$station['sunset']['day']['abbrv'] = $result[7];
		$station['sunset']['hour']['12'] = $result[8];
		$station['sunset']['hour']['24'] = $result[9];
		$station['sunset']['minute'] = $result[10];
		$station['sunset']['second'] = $result[11];
		$station['sunset']['am-pm'] = $result[12];
		
		// wet-bulb
		preg_match("/<aws:wet-bulb units=\"(.*?)\">([0-9\.\-]*?)<\/aws:wet-bulb>/", $file, $result);
		$station['wet-bulb']['units'] = str_replace('&amp;', '&', $result[1]);
		$station['wet-bulb']['value'] = $result[2];
		
		// wind
		preg_match("/<aws:wind-speed units=\"(.*?)\">([0-9\.]*?)<\/aws:wind-speed><aws:wind-speed-avg units=\"(.*?)\">([0-9\.]*?)<\/aws:wind-speed-avg><aws:wind-direction>(\w*?)<\/aws:wind-direction><aws:wind-direction-avg>(\w*?)<\/aws:wind-direction-avg>/", $file, $result);
		$station['wind']['units'] = $result[1];
		$station['wind']['value'] = $result[2];
		$station['wind']['direction'] = $result[5];
		$station['wind']['avg']['units'] = $result[3];
		$station['wind']['avg']['value'] = $result[4];
		$station['wind']['avg']['direction'] = $result[6];
		
		// If we are caching, store result
        $this->cache_result($cacheName, $station);
		// Return array
		return $station;
	}
	
	function wb_cameraList($code, $unitType = 0) {
        if (!is_array($code)) {
	        $this->lastError = "Code must be sent in array";
	    	return false;
		}
		if (array_key_exists('zipCode', $code)) {
            $url = "http://".strtolower($this->wb_key).".api.wxbug.net/getCamerasXML.aspx?acode=".strtoupper($this->wb_key)."&zipCode={$code['zipCode']}";
        } else if (array_key_exists('lat', $code) && array_key_exists('long', $code)) {
            $url = "http://".strtolower($this->wb_key).".api.wxbug.net/getCamerasXML.aspx?acode=".strtoupper($this->wb_key)."&lat={$code['lat']}&long={$code['long']}";
        } else {
            $this->lastError = "Must send either zipCode or lat/long";
	    	return false;
        }
        $url .= "&unitType=$unitType";
        $this->lastURL = $url;
		$cacheName = md5($url);
				
		/* if we are using caching, first look for a cache response */
		if ($cache = $this->cache_check($cacheName, false)) {
            return $cache;
        }
        
        try {
            $file = file_get_contents($url);
            if (strpos($file, '<?xml version="1.0" encoding="utf-8"?>') === false) {
                throw new Exception('Malformed XML');
            }
            // If we were successful, remove cache
            $this->cache_remove($cacheName);
        } catch (Exception $ex) {
            $this->lastError = $ex->getMessage();
            if ($cache = $this->cache_check($cacheName, true)) {
                return $cache;
            } else {
                return false;
            }
        }
		
    	$cameras = array();
    	preg_match_all("/<aws:camera id=\"([A-Z]{5}?)\" name=\"(.*?)\" city=\"(.*?)\" state=\"[A-Z]*?\" zipcode=\"([0-9]*?)\" distance=\"([0-9\.]*?)\" Unit=\"(.*?)\"/", $file, $results);
    	
    	$attributes = array(1=>'id', 'name', 'city', 'state', 'zipcode', 'distance', 'unit');
    	foreach ($results as $key=>$line) {
            if ($key == 0) {
                continue;
            }
            foreach ($line as $id=>$item) {
                $cameras[$id][$attributes[$key]] = $item;
            }
        }
        
        // If we are caching, store result
        $this->cache_result($cacheName, $cameras);
        // Return array
		return $cameras;
    }
	
  /**
   * Internal function that caches result
   *
   * @param string $request
   * @param string $response
   * @return boolean
   */
	function cache_result($request, $response) {
        if ($this->cache == true) {
            $request = mysql_real_escape_string($request);
            $response = mysql_real_escape_string(serialize($response));
            mysql_query("INSERT INTO `".self::CACHE_TABLE."` (`request`, `response`, `time`) VALUES ('$request', '$response', '".time()."')") or ($this->lastMySQLError = mysql_error());
        }
        return true;
    }
    
  /**
   * Internal function that checks if cache exists
   *
   * @param string $request
   * @param bool $useOld
   * @return mixed
   */
    function cache_check($request, $useOld = false) {
        if ($this->cache == true) {
            $request = mysql_real_escape_string($request);
            $sql = mysql_query("SELECT * FROM `".self::CACHE_TABLE."` WHERE `request` = '$request';") or ($this->lastMySQLError = mysql_error());
    		if ($row = mysql_fetch_assoc($sql)) {
    			if ($useOld || time() - $row['time'] < self::CACHE_TIME) {
    				return unserialize($row['response']);
    			}
    		}
        }
        return false;
    }
    
  /**
   * Internal function to clear cache out of database
   *
   * @param string $request
   * @return bool
   */
    function cache_remove($request) {
        if ($this->cache == true) {
            $request = mysql_real_escape_string($request);
            mysql_query("DELETE FROM `".self::CACHE_TABLE."` WHERE `request` = '$request';") or ($this->lastMySQLError = mysql_error());
        }
        return true;
    }
	
}