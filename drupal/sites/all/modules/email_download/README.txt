-- SUMMARY --

A tiny module to make downloads only available to people who submit their email address to the site in case the site creator doesn't want visitors to be required to create an account. The email is checked for validity and a cookie with a hard to guess authentication string is stored in a browser cookie. The next time (and of course this first time) the user makes a download, the cookie is checked and the user can proceed. This of course requires cookies to be on on the client side.

-- INSTALLATION --

 * Install as usual.

-- CONFIGURATION --

 * Configure user permissions
 * Configure the email_download module at admin/settings/email_download
 * Add files for download at admin/content/email_download

-- CONTACT --

Current maintainer:
 * Scott Hadfield (hadsie) - http://drupal.org/user/67093,
                             http://scotthadfield.ca
