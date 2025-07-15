#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <cgic.h>
#include <sqlite3.h>

#define GUESTBOOK_DB "cgi-bin/guestbook/guestbook.db"

char* escapeHtml(const char* input) {
    if (input == NULL) {
        return NULL;
    }

    size_t input_len = strlen(input);
    size_t output_len = input_len; // Initial estimate, will increase with escapes
    for (size_t i = 0; i < input_len; ++i) {
        switch (input[i]) {
            case '<': output_len += 3; break;
            case '>': output_len += 3; break;
            case '&': output_len += 4; break;
            case '"': output_len += 5; break;
            case '\'': output_len += 5; break;
        }
    }

    char* output = (char*)malloc(output_len + 1);
    if (output == NULL) {
        return NULL; // Memory allocation failed
    }

    size_t j = 0;
    for (size_t i = 0; i < input_len; ++i) {
        switch (input[i]) {
            case '<':
                strcpy(output + j, "&lt;");
                j += 4;
                break;
            case '>':
                strcpy(output + j, "&gt;");
                j += 4;
                break;
            case '&':
                strcpy(output + j, "&amp;");
                j += 5;
                break;
            case '"':
                strcpy(output + j, "&quot;");
                j += 6;
                break;
            case '\'':
                strcpy(output + j, "&#39;");
                j += 5;
                break;
            default:
                output[j++] = input[i];
        }
    }
    output[j] = '\0';
    return output;
}

void showForm() {

	fprintf(cgiOut,"<section class='post'>");

	fprintf(cgiOut,"<div class='title'><b>Sign Guestbook</b></div>");
	fprintf(cgiOut,"<hr>");
	
	fprintf(cgiOut,"<form action='' method='post'>");
	fprintf(cgiOut,"<font face='monospace'>");	
	fprintf(cgiOut,"<table width='100%%'>");
	fprintf(cgiOut,"<tr>");
	fprintf(cgiOut,"<td>Name</td>"); 
	fprintf(cgiOut,"<td><input style='width:100%%;font-size:12pt;' name='name' type='text' size='16' maxlength='32' required></td>");
	fprintf(cgiOut,"</tr>");
	fprintf(cgiOut,"<tr>");
	fprintf(cgiOut,"<td>Message</td>");
	fprintf(cgiOut,"<td><textarea style='width:100%%;font-size:12pt;resize:none;' name='message' cols='16' rows='8' maxlength='1024' required></textarea></td>");
	fprintf(cgiOut,"</tr>");
	fprintf(cgiOut,"<tr>");
	fprintf(cgiOut,
		"<td align='right' colspan='2'><input style='font-size:12pt;' type='reset' value='Clear'>&nbsp;"
		"<input style='font-size:12pt;' name='sign' type='submit' value='Sign'></td>"
	);
	fprintf(cgiOut,"</tr>");
	fprintf(cgiOut,"</table>");
	fprintf(cgiOut,"</font>");
	fprintf(cgiOut,"</form>");
	
	fprintf(cgiOut,"</section>");
}

int callback(void *NotUsed, int argc, char **argv, char **azColName) {
	NotUsed = 0;

	fprintf(cgiOut,"<section class='post'>");
	fprintf(cgiOut,"<div class='datetime'>%s</div>", argv[2] ? escapeHtml(argv[2]) : "NULL"); 
	fprintf(cgiOut,"<div class='name'>%s</div>", argv[0] ? escapeHtml(argv[0]) : "NULL");
	fprintf(cgiOut,"<hr>");
	fprintf(cgiOut,"<p>%s</p>", argv[1] ? escapeHtml(argv[1]) : "NULL"); 	
	fprintf(cgiOut,"</section>");

  return 0;
}

int viewGuestBook() {
	sqlite3 *db;
	char *err_msg = 0;
	sqlite3_stmt *res;

  int rc = sqlite3_open(GUESTBOOK_DB, &db);

  if (rc != SQLITE_OK) {
      fprintf(stderr, "Cannot open database: %s\n",sqlite3_errmsg(db));
      sqlite3_close(db);
      return 1;
  }

  char *sql = "SELECT name,message,strftime('%Y-%m-%d %l:%M:%S %p'	,datetime) as datetime FROM guestbook ORDER BY id DESC";

  rc = sqlite3_exec(db, sql, callback, 0, &err_msg);

  if (rc != SQLITE_OK ) {
    fprintf(stderr, "Failed to select data\n");
    fprintf(stderr, "SQL error: %s\n", err_msg);
    sqlite3_free(err_msg);
    sqlite3_close(db);
    return 1;
  }

  sqlite3_close(db);

  return 0;
}

int signGuestBook() {
	sqlite3 *db;
	char *err_msg = 0;
	sqlite3_stmt *res;

	char name[33];
	char message[1025];

	if(cgiFormStringNoNewlines("name",name,33)==cgiFormEmpty) return 1;
	if(cgiFormString("message",message,1025)==cgiFormEmpty) return 1;

  int rc = sqlite3_open(GUESTBOOK_DB, &db);

  if (rc != SQLITE_OK) {
    fprintf(stderr, "Cannot open database: %s\n", sqlite3_errmsg(db));
    sqlite3_close(db);
    return 1;
  }

  char *sql =  "INSERT INTO guestbook(name,message) VALUES(?,?);";

  rc = sqlite3_prepare_v2(db, sql, -1, &res, 0);

	if (rc != SQLITE_OK) {
		fprintf(stderr,"ERROR preparing sql: %s\n", sqlite3_errmsg(db));
		sqlite3_close(db);
		return 1;
	}

 	sqlite3_bind_text(res, 1, name, 33, SQLITE_STATIC);
	sqlite3_bind_text(res, 2, message, 1025, SQLITE_STATIC);

	if (sqlite3_step(res) != SQLITE_DONE) {
		fprintf(stderr,"ERROR executing stmt: %s\n", sqlite3_errmsg(db));
		sqlite3_close(db);
		return 1;
	}

	sqlite3_clear_bindings(res);
	rc = sqlite3_reset(res);
	if (rc != SQLITE_OK) {
		sqlite3_close(db);
		return 1;
	}

	sqlite3_finalize(res);
	sqlite3_close(db);

	return 0;
}

int cgiMain() {
	cgiHeaderContentType("text/html");

	fprintf(cgiOut,"<html>");
	fprintf(cgiOut,"<head>");

	fprintf(cgiOut,
		"<style>"
    "* { font-family: sans-serif; }"
    "body { background: #F0F0F0; color: navy; }"
    "header { color: white; background:navy;"
    	"text-align: center;"
    	"margin: 16px 16px 0 16px; padding: 16px;" 
		  "border-left: 4px solid white;" 
		  "border-right: 4px solid white;" 
      "border-top: 4px solid white;" 
      "border-bottom: none; border-radius:8px 8px 0 0;"
      "box-shadow: 4px 4px 8px #505050;}"    
    "main { color: navy; background: white;" 
      "margin: 0 16px 0 16px; padding: 8px;" 
      "border-left: 4px solid white;" 
      "border-right: 4px solid white;" 
      "border-top: none;"
      "border-bottom: none;"
      "box-shadow: 4px 4px 8px #505050;}"
    "footer { color: white;"
    	"background: navy;" 
    	"text-align: center;" 
      "font-weight: bold;"
      "margin: 0px 16px 16px 16px;"
      "padding: 16px;" 
      "border-left: 4px solid white;" 
      "border-right: 4px solid white;" 
      "border-top: none;"
      "border-bottom: 4px solid white;"
      "border-radius: 0 0 8px 8px;"
      "box-shadow: 4px 4px 8px #505050;}"
    "section.post { margin: 32px 8px 32px 8px;"
    	"padding: 8px;"
    	"border: 1px solid navy;" 
    	"border-radius: 8px;" 
    	"box-shadow: 2px 2px 4px #505050 inset;"
			"text-align: justify;}"
    "section.post p.indent { text-indent: 50px;}"
    ".title {}"
    ".datetime { float: right;}"
    ".clear { clear: both;}"
    ".comments { float: right; }"
    "hr { border: 1px solid navy; }"
    "#container {max-width: 800px; margin: 0 auto;}"
    "</style>");

	fprintf(cgiOut,"</head>");
	fprintf(cgiOut,"<body>");

	fprintf(cgiOut,"<div id='container'>");

	if(cgiFormSubmitClicked("sign") == cgiFormSuccess) {
		signGuestBook();
	}

	fprintf(cgiOut,"<header>");
	fprintf(cgiOut,"<h1>Guestbook</h1>");
	fprintf(cgiOut,"</header>");

	fprintf(cgiOut,"<main>");

	showForm();
	viewGuestBook();

	fprintf(cgiOut,"</main>");

	fprintf(cgiOut,"<footer>");
  fprintf(cgiOut,"<h4>Since 2022-09-01 by FriaElAgua</h4>");
	fprintf(cgiOut,"</footer>");

	fprintf(cgiOut,"</div>");



	fprintf(cgiOut,"</body>");
	fprintf(cgiOut,"</html>");



	return 0;
}

