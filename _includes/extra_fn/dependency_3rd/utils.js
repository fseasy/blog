{% unless loadUtilsDone %}

// wait for final event, avoid call too frequently in continuous event(e.g. resize, scroll)
// copy from https://stackoverflow.com/a/4541963/4869018
var waitForFinalEvent = (function () {
    var timers = {};
    return function (callback, ms, uniqueId) {
      if (!uniqueId) {
        uniqueId = "Don't call this twice without a uniqueId";
      }
      if (timers[uniqueId]) {
        clearTimeout (timers[uniqueId]);
      }
      timers[uniqueId] = setTimeout(callback, ms);
    };
})();

{% assign loadUtilsDone = true %}
{% endunless %}