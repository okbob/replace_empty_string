/* contrib/spi/replace_empty_string--1.0.sql */

-- complain if script is sourced in psql, rather than via CREATE EXTENSION
\echo Use "CREATE EXTENSION replace_empty_string" to load this file. \quit

CREATE FUNCTION replace_empty_string()
RETURNS trigger
AS 'MODULE_PATHNAME'
LANGUAGE C;
