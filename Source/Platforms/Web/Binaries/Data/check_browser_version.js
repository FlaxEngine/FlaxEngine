/**
 * @license
 * Copyright 2024 The Emscripten Authors
 * SPDX-License-Identifier: MIT
 */

(function() {
  // "30.0.0" -> 300000
  function humanReadableVersionToPacked(str) {
    str = str.split('-')[0]; // Remove any trailing part from e.g. "12.53.3-alpha"
    var vers = str.split('.').slice(0, 3);
    while(vers.length < 3) vers.push('00');
    vers = vers.map((n, i, arr) => n.padStart(2, '0'));
    return vers.join('');
  }
  // 300000 -> "30.0.0"
  var packedVersionToHumanReadable = n => [n / 10000 | 0, (n / 100 | 0) % 100, n % 100].join('.');

  var userAgent = typeof navigator !== 'undefined' && navigator.userAgent;
  if (!userAgent) {
    return;
  }

  var currentSafariVersion = userAgent.includes("Safari/") && !userAgent.includes("Chrome/") && userAgent.match(/Version\/(\d+\.?\d*\.?\d*)/) ? humanReadableVersionToPacked(userAgent.match(/Version\/(\d+\.?\d*\.?\d*)/)[1]) : TARGET_NOT_SUPPORTED;
  if (currentSafariVersion < MIN_SAFARI_VERSION) {
    console.log(navigator.userAgent);
    var msg = `Required Safari v${ packedVersionToHumanReadable(MIN_SAFARI_VERSION) }, detected v${ packedVersionToHumanReadable(currentSafariVersion) }.`;
    alert(msg)
    throw new Error(msg);
  }

  var currentFirefoxVersion = userAgent.match(/Firefox\/(\d+(?:\.\d+)?)/) ? parseFloat(userAgent.match(/Firefox\/(\d+(?:\.\d+)?)/)[1]) : TARGET_NOT_SUPPORTED;
  if (currentFirefoxVersion < MIN_FIREFOX_VERSION) {
    console.log(navigator.userAgent);
	var msg = `Required Firefox vMIN_FIREFOX_VERSION, detected v${currentFirefoxVersion}.`;
    alert(msg)
    throw new Error(msg);
  }

  var currentChromeVersion = userAgent.match(/Chrome\/(\d+(?:\.\d+)?)/) ? parseFloat(userAgent.match(/Chrome\/(\d+(?:\.\d+)?)/)[1]) : TARGET_NOT_SUPPORTED;
  if (currentChromeVersion < MIN_CHROME_VERSION) {
    console.log(navigator.userAgent);
    var msg = `Required Chrome vMIN_CHROME_VERSION, detected v${currentChromeVersion}.`;
    alert(msg)
    throw new Error(msg);
  }
})();
