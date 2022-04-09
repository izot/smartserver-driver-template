/**
  *******************************************************************************
  * @file    csvin.h
  * @author  Glen Riley
  *
  * @brief   This file contains APIs useful for reading CSV files.
  *           OpenCsv() - starts the process of reading a CSV.
  *           OpenCsvMeta() - same as OpenCsv() except that it is for meta data in the CSV.
  *           ReadCsv() - reads next line of a CSV file.
  *           GetCsvItem() - returns an item from a line returned by ReadCsv() corresponding to a particular heading.
  *******************************************************************************
  * @attention
  *   Copyright 2019 - 2022 Dialog Semiconductor.  All Rights Reserved.
  *******************************************************************************
  **/

#pragma once

#include <stdio.h>

#define MAX_FIELDS 100

typedef struct {
    int count;
    char * fields[MAX_FIELDS];
} Fields;

typedef struct {
    FILE *f;
    bool meta;  // 1 => get meta data rows
    Fields current;
    Fields headings;
} Csv;

int ReadCsv(Csv * p);
Csv * OpenCsv(const char * name);
Csv * OpenCsvMeta(const char * name);
char * GetCsvItem(Csv *p, const char * heading);
char * GetCsvField(Csv *p, int i);
void CloseCsv(Csv *p);
void RemoveSpaces(char *str);

