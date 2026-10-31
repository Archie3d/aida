package body Derived_Profile_Model is
    function Make (Round : Boolean) return Item is
    begin
        if Round then
            return (True, 7);
        else
            return (False, 9);
        end if;
    end Make;
    function Read (Object : Alias_Circle) return Integer is
    begin
        Calls := Calls + 1;
        return Object.Radius;
    end Read;
    function Inspect (Object : Item) return Boolean is
    begin
        return Object.Round;
    end Inspect;
    procedure Touch (Object : in out Alias_Circle) is
    begin
        Calls := Calls + 1;
        Object.Radius := 8;
    end Touch;
    procedure Reset (Object : out Alias_Circle) is
    begin
        Calls := Calls + 1;
        Object := (True, 6);
    end Reset;
end Derived_Profile_Model;
