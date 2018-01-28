(function($){
  $(function(){

    // One time tab init
    // Set to true since we don't have any subtabs anymore
    var oneTimeTabInit = true;

    $('.button-collapse').sideNav();

    // https://github.com/Dogfalo/materialize/issues/2102#issuecomment-284171287
    $('.collapsible').collapsible({
      onOpen: function(el) {
        // https://github.com/Dogfalo/materialize/issues/2102
        window.dispatchEvent(new Event('resize'));
      }
    });
  
    // Similarly, for subtabs...
    // We only init once because otherwise the animation doesn't fire
    // anymore when resizes occur.
    $('ul.tabs').tabs({
      onShow: function(el) {
        // https://github.com/Dogfalo/materialize/issues/2102
        console.log("Tab change detected");
        if (!oneTimeTabInit) {
          window.dispatchEvent(new Event('resize'));
          oneTimeTabInit = true;
        }
        return true;
      }
    });
    
    // Custom HTML support for tooltips
    $('.tooltipped').tooltip({html: true});
  }); // end of document ready
})(jQuery); // end of jQuery name space
