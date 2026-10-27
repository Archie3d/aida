package body GenericClasswideLibrary is
    function Construct (Value : Integer := 3) return T renames Make;
    function Score (Object : T) return Integer is
    begin
        return Read (Object);
    end Score;
    function Clone (Object : T) return T is
    begin
        return Copy (Object);
    end Clone;
    procedure Reset (Object : in out T) is
    begin
        Object := Make (Value => 8);
    end Reset;
    procedure Renamed_Reset (Object : in out T) is
    begin
        Object := Construct (Value => 9);
    end Renamed_Reset;
    function Nested_Score (Object : U) return Integer is
    begin
        return Read (Object);
    end Nested_Score;
end GenericClasswideLibrary;
