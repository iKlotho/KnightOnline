/*
*/

#include <windows.h>
#include <tchar.h>

#include <sql.h>
#include <sqlext.h>
#include <sqltypes.h>

#include "config.h"
#include "fl/fl.h"
#include "fl/fl_window.h"
#include "fl/fl_hor_slider.h"
#include "fl/fl_toggle_button.h"
#include "fl/math.h"
#include "fl/fl_draw.H"
#include "fl/gl.h"
#include "fl/fl_gl_window.h"
#include "fl/fl_table_row.H"

const int _gl_width = 380;
const int _gl_height = 1024/3;

//-----------------------------------------------------------------------------
typedef struct {
	long Num;
} ITEM_ROW;

int item_table_count;
ITEM_ROW* item_table;

//-----------------------------------------------------------------------------
class shape_window: public Fl_Gl_Window {
private:
	void draw(void);
	void draw_overlay(void);

public:
	int sides;
	int overlay_sides;

public:
	shape_window(int x, int y, int w, int h, const char* l = NULL);
};

shape_window::shape_window(int x, int y, int w, int h, const char* l):
Fl_Gl_Window(x, y, w, h, l) {
	sides = overlay_sides = 6;
}

void shape_window::draw(void) {
	if(!valid()) {
		valid(1);
		glLoadIdentity();
		glViewport(0, 0, pixel_w(), pixel_h());
	}

	glClear(GL_COLOR_BUFFER_BIT);

	glBegin(GL_POLYGON);
	for(int i=0; i<sides; ++i) {
		double ang = i*2*M_PI/sides;
		glColor3f(float(i)/sides, float(i)/sides, float(i)/sides);
		glVertex3f(cos(ang), sin(ang), 0.0f);
	}
	glEnd();
}

void shape_window::draw_overlay(void) {
	if(!valid()) {
		valid(1);
		glLoadIdentity();
		glViewport(0, 0, pixel_w(), pixel_h());
	}

	gl_color(FL_RED);
	glBegin(GL_LINE_LOOP);
	for(int i=0; i<overlay_sides; ++i) {
		double ang = i*2*M_PI/overlay_sides;
		glVertex3f(cos(ang), sin(ang), 0.0f);
	}
	glEnd();
}

//-----------------------------------------------------------------------------
void sides_cb(Fl_Widget* widget, void* data) {
	shape_window* sw = (shape_window*)data;
	sw->sides = int(((Fl_Slider*)widget)->value());
	sw->redraw();
}

//-----------------------------------------------------------------------------
void overlay_sides_cb(Fl_Widget* widget, void* data) {
	shape_window* sw = (shape_window*)data;
	sw->overlay_sides = int(((Fl_Slider*)widget)->value());
	sw->redraw_overlay();
}

//-----------------------------------------------------------------------------
class ItemTableView: public Fl_Table_Row {
private:
	Fl_Color cell_bgcolor;
	Fl_Color cell_fgcolor;

protected:
	void draw_cell(TableContext context,
		int r = 0, int c = 0, int x = 0, int y = 0, int w = 0, int h = 0
	);

public:
	Fl_Color GetCellBGColor(void) const {return cell_bgcolor;}
	Fl_Color GetCellFGColor(void) const {return cell_fgcolor;}

	void SetCellBGColor(Fl_Color color) {cell_bgcolor = color;}
	void SetCellFGColor(Fl_Color color) {cell_fgcolor = color;}

public:
	ItemTableView(int x, int y, int w, int h, const char* l = NULL):
	Fl_Table_Row(x, y, w, h, l) {
		cell_bgcolor = FL_WHITE;
		cell_fgcolor = FL_BLACK;
	}
};

void ItemTableView::draw_cell(TableContext context,
	int r, int c, int x, int y, int w, int h
) {
	if(r>=item_table_count || c>=1) return;

	static char s[40];
	sprintf(s, "r:%d, c:%d", r, c);

	switch(context) {
		case CONTEXT_STARTPAGE:
			fl_font(FL_HELVETICA, 16);
			return;
		case CONTEXT_COL_HEADER:
			fl_push_clip(x, y, w, h);
			{
				fl_draw_box(FL_THIN_UP_BOX, x, y, w, h, col_header_color());
				fl_color(FL_BLACK);
				fl_draw(s, x, y, w, h, FL_ALIGN_CENTER);
			}
			fl_pop_clip();
			return;
		case CONTEXT_ROW_HEADER:
			fl_push_clip(x, y, w, h);
			{
				fl_draw_box(FL_THIN_UP_BOX, x, y, w, h, row_header_color());
				fl_color(FL_BLACK);
				fl_draw(s, x, y, w, h, FL_ALIGN_CENTER);
			}
			fl_pop_clip();
			return;
		case CONTEXT_CELL:
			sprintf(s, "%d", item_table[r].Num);

			fl_push_clip(x, y, w, h);
			{
				fl_color(row_selected(r) ? selection_color() : cell_bgcolor);
				fl_rectf(x, y, w, h);
				fl_color(cell_fgcolor);
				fl_draw(s, x, y, w, h, FL_ALIGN_CENTER);
				fl_color(color());
				fl_rect(x, y, w, h);
			}
			fl_pop_clip();
			return;
		case CONTEXT_TABLE:
			printf("Table Context Called?\n");
			return;
		case CONTEXT_ENDPAGE:
		case CONTEXT_RC_RESIZE:
		case CONTEXT_NONE:
			return;
	}
}

//-----------------------------------------------------------------------------
int main(int argc, char** argv) {

	//----
	SQLHANDLE hEnv, hConn;

	SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &hEnv);
	SQLSetEnvAttr(hEnv, SQL_ATTR_ODBC_VERSION, (void *)SQL_OV_ODBC3, 0);
	SQLAllocHandle(SQL_HANDLE_DBC, hEnv, &hConn);
	if(SQLConnect(hConn, _T("KN_online"), SQL_NTS, _T("knight"), SQL_NTS, _T("knight"), SQL_NTS) == SQL_ERROR) {
		printf("SQLConnect\n");
		system("pause");
		return -1;
	}

	//----
	SQLHANDLE hStmt;
	SQLAllocHandle(SQL_HANDLE_STMT, hConn, &hStmt);
	if(SQLExecDirect(hStmt, _T("SELECT TOP(10) Num FROM ITEM;"), SQL_NTS) == SQL_ERROR) {
		printf("SQLExecDirect\n");
		system("pause");
		return -1;
	}

	long item_Num = 0;
	SQLINTEGER cbData;
	while(SQLFetch(hStmt) == SQL_SUCCESS) {
		item_table = (ITEM_ROW*)realloc(item_table, ++item_table_count*sizeof(ITEM_ROW));
		ITEM_ROW* item = &item_table[item_table_count-1];
		memset(item, 0x00, sizeof(ITEM_ROW));

		SQLGetData(hStmt, 1, SQL_C_LONG, &(item->Num), sizeof(long), &cbData);
	}

	SQLFreeHandle(SQL_HANDLE_STMT, hStmt);
	//----

	SQLDisconnect(hConn);
	SQLFreeHandle(SQL_HANDLE_DBC, hConn);
	SQLFreeHandle(SQL_HANDLE_ENV, hEnv);
	//----



	Fl::use_high_res_GL(true);
	Fl_Window window(1024, 720);

	shape_window sw(window.w()-(_gl_width+10), 10, _gl_width, _gl_height);

	ItemTableView demo_table(10, 10, window.w()-(_gl_width+30), _gl_height);
	demo_table.selection_color(FL_YELLOW);
	demo_table.when(FL_WHEN_RELEASE|FL_WHEN_CHANGED);
	demo_table.table_box(FL_NO_BOX);
	demo_table.col_resize_min(4);
	demo_table.row_resize_min(4);

	demo_table.row_header(true);
	demo_table.row_header_width(60);
	demo_table.row_resize(true);
	demo_table.rows(item_table_count);
	demo_table.row_height_all(20);

	demo_table.col_header(true);
	demo_table.col_header_height(25);
	demo_table.col_resize(true);
	demo_table.cols(1);
	demo_table.col_width_all(80);

	window.end();
	window.show(argc, argv);

	sw.show();
	sw.redraw_overlay();

	//system("pause");
	return Fl::run();
}