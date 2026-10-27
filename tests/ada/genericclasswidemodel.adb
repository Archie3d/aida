package body GenericClasswideModel is
    function Read (Object : Root) return Integer is
    begin
        return Object.Value;
    end Read;
    procedure Set (Object : in out Root; Value : Integer) is
    begin
        Object.Value := Value;
    end Set;
    function Make (Value : Integer := 3) return Root is
    begin
        return (Value => Value);
    end Make;
    function Copy (Object : Root) return Root is
    begin
        return Object;
    end Copy;
    procedure Combine (Left : Root; Right : Root) is
    begin
        null;
    end Combine;
    function Read (Hidden_Object : Child) return Integer is
    begin
        return Hidden_Object.Value + Hidden_Object.Extra;
    end Read;
    procedure Set (Object : in out Child; Value : Integer) is
    begin
        Object.Value := Value + 1;
    end Set;
    function Make (Hidden_Value : Integer := 4) return Child is
    begin
        return (Root'(Value => Hidden_Value) with Extra => 20);
    end Make;
    function Copy (Object : Child) return Child is
    begin
        return Object;
    end Copy;
end GenericClasswideModel;
