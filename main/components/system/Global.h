//
// Created by long.nguyenviet on 8/28/25.
//

#pragma once
#include "dto/bookinfo.h"
static std::string BOOK_BASE_PATH = "/sdcard/books/";
static bookinfo_t* currentBook;

inline void global_current_book(bookinfo_t* book)
{
    currentBook = book;
}

inline void global_set_current_book(bookinfo_t* book)
{
    currentBook = book;
}