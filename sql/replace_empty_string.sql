DROP TABLE IF EXISTS res;

CREATE EXTENSION replace_empty_string;

CREATE TABLE res (
	id		int4,
	idesc		text,
	test1		varchar,
	test2		text
);

CREATE TRIGGER res_replace_empty_string
	BEFORE UPDATE OR INSERT ON res
	FOR EACH ROW
	EXECUTE PROCEDURE replace_empty_string ();

INSERT INTO res VALUES (1, 'first', NULL, '');
INSERT INTO res VALUES (2, NULL, '', 'Hello');

\pset null ****

SELECT * FROM res;

UPDATE res SET idesc = ''
	WHERE id = 1;

SELECT * FROM res;

DROP TRIGGER res_replace_empty_string ON res;

-- print warnings
CREATE TRIGGER res_replace_empty_string
	BEFORE UPDATE OR INSERT ON res
	FOR EACH ROW
	EXECUTE PROCEDURE replace_empty_string ('on');

INSERT INTO res VALUES (3, 'first', NULL, '');
INSERT INTO res VALUES (4, NULL, '', 'Hello');

SELECT * FROM res;

UPDATE res SET idesc = ''
	WHERE id = 3;
