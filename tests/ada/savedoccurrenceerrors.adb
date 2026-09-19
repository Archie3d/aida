with Ada.Exceptions; use Ada.Exceptions;
procedure Savedoccurrenceerrors is
    procedure Read_Only (Source : Exception_Occurrence) is
    begin
        Save_Occurrence (Source, Null_Occurrence);
    end Read_Only;
    type Wrapper is record
        Item : Exception_Occurrence;
    end record;
    procedure Read_Only_Field (Box : Wrapper) is
    begin
        Save_Occurrence (Box.Item, Null_Occurrence);
    end Read_Only_Field;
begin
    Save_Occurrence (Null_Occurrence, Null_Occurrence);
    Save_Occurrence (Ada.Exceptions.Null_Occurrence, Null_Occurrence);
    begin
        raise Constraint_Error;
    exception
        when E : others => Save_Occurrence (E, Null_Occurrence);
    end;
end Savedoccurrenceerrors;
