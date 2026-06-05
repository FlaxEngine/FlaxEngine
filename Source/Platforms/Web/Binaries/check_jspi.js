if (!('Suspending' in WebAssembly)) {
  console.log(navigator.userAgent);
  alert(`JSPI is not supported in this browser.`)
  throw new Error(`JSPI is not supported in this browser.`);
}
