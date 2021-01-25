$(document).ready(function ()
{
    $("#loginForm").append('<p id="pMessageWrongPassword" class="login-error-message" style="display: none"></p>');
    $("#loginForm").append('<p id="pMessageUserAlreadyLogged" class="login-error-message" style="display: none"></p>');
    $("#loginForm").append('<p id="pMessageLoginFailed" class="login-error-message" style="display: none"></p>');

    // Check if cookie with language setting already exists
    if (Cookies.get('webLang') == null)
    {
        changeLanguage('en'); // default language
    } else
    {
        changeLanguage(Cookies.get('webLang'));
    }

    $("div#navigation").removeClass("hidden");
    $("div#navigation-mobile").removeClass("hidden");

    hideDimScreen();

    $("div#content").removeClass("hidden");
    $("div#footer").removeClass("hidden");

//    $("#content").css("min-height", $(window).height() - 280);
//    $("#content").css("height", $(window).height() - 280);
}
);

$(window).on('resize', function ()
{
    if ($('#navigation').is(':visible'))
    {
        $("#navigation-mobile").slideUp(200, "linear");
        $("#hamburger").removeClass("change");
    }

});

$('#loginForm').submit(function (e)
{
    e.preventDefault();

    showDimScreenWaitingCircle();
    $("#pMessageWrongPassword").hide();
    $("#pMessageUserAlreadyLogged").hide();
    $("#pMessageLoginFailed").hide();

    var pass = $("#inputLoginPassword").val();
    var sessid = createSession(pass);

    $("#sessid").remove();
    $('<input>').attr({
        type: 'hidden',
        id: 'sessid',
        name: 'sessid',
        value: sessid
    }).appendTo('#loginForm');

    // send ajax
    $.ajax({
        url: '/api/login', // url where to submit the request
        type: "POST", // type of action POST || GET
        dataType: 'json', // data type
        data: $("#loginForm").serialize(), // post data || get data
        success: function (result)
        {
            switch (result.err)
            {
                case 0:
                    $(location).attr('href', 'index.html');
                    break;
                case 1:
                    $("#pMessageWrongPassword").show();
                    hideDimScreen();
                    break;
                case 2:
                    $("#pMessageUserAlreadyLogged").show();
                    hideDimScreen();
                    break;
                case 3:
                default:
                    $("#pMessageLoginFailed").show();
                    hideDimScreen();
                    break;
            }

        },
        error: function (xhr, resp, text)
        {
            console.log(xhr, resp, text);

            hideDimScreen();
            $('#sessid').remove();
            return false;
        }
    })
});


$("#langEn").click(function ()
{
    changeLanguage('en');
});

$("#langDe").click(function ()
{
    changeLanguage('de');
});

$("#langmobileEn").click(function ()
{
    changeLanguage('en');
    hideMobileMenu();
});

$("#langmobileDe").click(function ()
{
    changeLanguage('de');
    hideMobileMenu();
});

$("#hamburger").click(function ()
{
    this.classList.toggle("change");
    $("#navigation-mobile").slideToggle(200, "linear");
});

$('#selectLanguage').change(function ()
{
    var value = $(this).val();

    if (value == 'de')
    {
        changeLanguage('de');

    } else
    {
        changeLanguage('en');
    }

    $(this).blur();
});

/**
 * Update website texts based on language version
 * @param {object} data Translations from JSON
 * @returns {undefined}
 */
function updateTexts(data)
{
    $("#buttonLogin").text(data.buttonLogin);

    // placeholder
    $("#inputLoginPassword").attr("placeholder", data.inputLoginPassword);

    // paragraph
    $("#pMessageWrongPassword").text(data.pMessageWrongPassword);
    $("#pMessageUserAlreadyLogged").text(data.pMessageUserAlreadyLogged);
    $("#pMessageLoginFailed").text(data.pMessageLoginFailed);
}

/**
 * Change website language
 * @param {type} language Code of language (en/de)
 * @returns {undefined}
 */
function changeLanguage(language)
{

}

/**
 * Create session id and store it in cookie sessid
 * @param {type} input
 * @returns {createSession.hash} Hash which is used as session ID
 */
function createSession(input)
{
    var hash = sha1(input + Date.now());    // get random sha1 hash as session ID
    Cookies.set('sessid', hash, {expires: 1});
    return hash;
}

/**
 * Close and hide mobile menu
 * @returns {undefined}
 */
function hideMobileMenu()
{
    $("#navigation-mobile").removeClass("opened");
    $("#navigation-mobile").slideUp(200, "linear");

    $("#hamburger").removeClass("change");
}

/**
 * Show dim screen with waiting circle animation and blur container div
 * @returns {undefined}
 */
function showDimScreenWaitingCircle()
{
    $("#container").addClass("blur");
    $("#loadingCircle").show();
    $("#dimScreen").show();
    $("body").css("overflow", "hidden");
}

/**
 * Hide dim screen
 * @returns {undefined}
 */
function hideDimScreen()
{
    $("body").css("overflow", "visible");
    $("#container").removeClass("blur");
    $("#dimScreen").hide();
}



                