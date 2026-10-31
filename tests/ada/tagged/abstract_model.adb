package body Abstract_Model is
    function Default_Read (Self : Root := Make) return Integer is
    begin
        return Read (Root'Class (Self));
    end Default_Read;
    function Sum (Self : Root; Other : Root := Make) return Integer is
    begin
        return Read (Root'Class (Self)) + Read (Root'Class (Other));
    end Sum;
    Hidden : Opaque;
    package body Leaves is
        overriding procedure Change (Self : in out Leaf) is
        begin
            Self.Value := Self.Value + 4;
        end Change;
        overriding procedure Via_Alias (Self : in out Leaf) is
        begin
            Change (Self);
        end Via_Alias;
        overriding function Read (Self : Leaf) return Integer is
        begin
            return Self.Value;
        end Read;
        overriding function Probe (Self : access Leaf) return Integer is
        begin
            return Self.Value;
        end Probe;
        overriding function Make return Leaf is
        begin
            return (Middle with null record);
        end Make;
        overriding function "+" (Left, Right : Leaf) return Leaf is
            Result : Leaf;
        begin
            Result.Value := Left.Value + Right.Value;
            return Result;
        end "+";
    end Leaves;
end Abstract_Model;
