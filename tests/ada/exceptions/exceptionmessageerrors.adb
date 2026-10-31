with Ada.Exceptions; use Ada.Exceptions;
procedure Exceptionmessageerrors is
    E : exception;
    I : Integer := 0;
    Copy : Exception_Occurrence := Null_Occurrence;
    function Escape return Exception_Occurrence is
    begin
        return Null_Occurrence;
    end Escape;
    type Occurrence_Access is access Exception_Occurrence;
    Heap_Copy : Occurrence_Access := new Exception_Occurrence'(Null_Occurrence);
begin
    raise E with 42;
    I := I'Identity;
    I := E'Identity (1);
end Exceptionmessageerrors;
