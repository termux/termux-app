package com.termux.terminal;

public class Cursor {
    private int row;
    private int column;

    public Cursor(int row, int column) {
        this.row = row;
        this.column = column;
    }

    public int getRow() {
        return row;
    }

    public int getColumn() {
        return column;
    }

    public void setCursor(Cursor cursor) {
        this.row = cursor.getRow();
        this.column = cursor.getColumn();
    }

    public void setRow(int row) {
        this.row = row;
    }

    public void setColumn(int column) {
        this.column = column;
    }

    public void addToRow(int adder) {
        this.row += adder;
    }

    public void addToColumn(int adder) {
        this.column += adder;
    }
}
