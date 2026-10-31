package body Callback_Inherited_Parent is
    function Read (Object : Root) return Integer is
    begin
        return Object.Value;
    end Read;
    procedure Set (Object : in out Root; Value : Integer) is
    begin
        Object.Value := Value;
    end Set;
    function Create (Value : Integer) return Root is
    begin
        return (Value => Value);
    end Create;
end Callback_Inherited_Parent;
