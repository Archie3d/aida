procedure RuntimeComponentErrors is
    N : Integer := 3;
    type Missing_Record is record
        Data : String;
    end record;
    type Missing_Array is array (1 .. 2) of String;
    type Tagged_Record is tagged record
        Data : String (1 .. N);
    end record;
    type Dynamic_Record is record
        Data : String (1 .. N);
    end record;
    for Dynamic_Record'Size use 128;
    type Pointer is access String;
    Missing : Pointer := new String;
    subtype Text is String (1 .. N);
    Bad : Pointer := new Text (1 .. N);
begin
    null;
end RuntimeComponentErrors;
