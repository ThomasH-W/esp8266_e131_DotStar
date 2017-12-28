jQuery( document ).ready(function( $ ) {
  $(document).ready( updateForm );
});

// update the value of form elements
function updateForm() {
$.getJSON( "json", function() {
  //alert( "success" );
})
  .done(function(data) {
    $.each(data, function(key, value) {
      $( document.getElementById("settings-form").elements[key] ).val(value)
    });
    //alert( "done" );
  })
  .fail(function() {
    //alert( "error" );
  })
  .always(function() {
    //alert( "finished" );
  });
}

//function postForm() {
//   $('#settings-form').submit(function(e){
//       e.preventDefault();
//       var form = $(this);
//       var post_url = form.attr('action');
//       var post_data = form.serialize();
//       $('#loader3', form).html('<img src="/cat.jpg" />       Please wait...');
//       $.ajax({
//           type: 'POST',
//           url: post_url,
//           data: post_data,
//           success: function(msg) {
//               $(form).fadeOut(800, function(){
//                   form.html(msg).fadeIn().delay(2000);
//               });
//           }
//       });
//   });
//}

