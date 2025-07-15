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

	fprintf(cgiOut,"<div class='title'>Sign Guestbook</div>");
	fprintf(cgiOut,"<hr>");
	
	fprintf(cgiOut,"<form action='' method='post'>");
	fprintf(cgiOut,"<table width='100%%'>");
	fprintf(cgiOut,"<tr>");
	fprintf(cgiOut,"<td><div class='title'>Name</div></td>");
	fprintf(cgiOut,"<td><input style='width:100%%;' name='name' type='text' size='16' maxlength='32' required></td>");
	fprintf(cgiOut,"</tr>");
	fprintf(cgiOut,"<tr>");
	fprintf(cgiOut,"<td><div class='title'>Message</div></td>");
	fprintf(cgiOut,"<td><textarea style='width:100%%;resize:none;' name='message' cols='16' rows='8' maxlength='1024' required></textarea></td>");
	fprintf(cgiOut,"</tr>");
	fprintf(cgiOut,"<tr>");
	fprintf(cgiOut,
		"<td align='right' colspan='2'>"
		"<input class='button' type='reset' value='Clear'>&nbsp;"
		"<input class='button' name='sign' type='submit' value='Sign'>"
		"</td>"
	);
	fprintf(cgiOut,"</tr>");
	fprintf(cgiOut,"</table>");
	fprintf(cgiOut,"</form>");

	fprintf(cgiOut,"</section>");
}

int callback(void *NotUsed, int argc, char **argv, char **azColName) {
	NotUsed = 0;

	fprintf(cgiOut,"<section class='post'>");
	fprintf(cgiOut,"<div class='datetime'>%s</div>", argv[2] ? escapeHtml(argv[2]) : "NULL"); 	fprintf(cgiOut,"<div class='name'>%s</div>", argv[0] ? escapeHtml(argv[0]) : "NULL");
	fprintf(cgiOut,"<div class='clear'></div>");
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
	fprintf(cgiOut,"<meta name='viewport' content='width=device-width, initial-scale=1.0' />");
	fprintf(cgiOut,"<title>POCS - Guestbook</title>");
	fprintf(cgiOut,"<link rel='stylesheet' href='../../../style.css'>");
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

