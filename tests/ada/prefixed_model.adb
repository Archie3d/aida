package body Prefixed_Model is
    procedure Add (Object : in out Root; Amount : Integer := 1) is
    begin
        Object.Value := Object.Value + Amount;
    end Add;
    procedure Add (Object : in out Root; Twice : Boolean) is
    begin
        if Twice then Object.Value := Object.Value * 2; end if;
    end Add;
    function Read (Object : Root) return Integer is
    begin
        return Object.Value;
    end Read;
    function Read (Object : Root) return Boolean is
    begin
        return Object.Value > 0;
    end Read;
    function Via_Access (Object : access constant Root) return Integer is
    begin
        return Object.Value;
    end Via_Access;
    procedure Set (Object : access Root; Value : Integer) is
    begin
        Object.Value := Value;
    end Set;
    function Copy (Object : Root) return Root is
    begin
        return Object;
    end Copy;
    function Wide_Read (Object : Root'Class; Amount : Integer := 0) return Integer is
    begin
        return Object.Value + Amount;
    end Wide_Read;
    function Read (Object : Child) return Integer is
    begin
        return Object.Value + Object.Extra;
    end Read;
    function Via_Access (Object : access constant Child) return Integer is
    begin
        return Object.Value + Object.Extra;
    end Via_Access;
    function Copy (Object : Child) return Child is
    begin
        return Object;
    end Copy;
    procedure Named_First (Object : Root_Pointer) is
    begin
        null;
    end Named_First;
    procedure Second (Value : Integer; Object : Root) is
    begin
        null;
    end Second;
    function Read (Object : Other) return Character is
    begin
        return 'Q';
    end Read;
    function Read (Object : Private_Root) return Integer is
    begin
        return Object.Value;
    end Read;
    function Hidden (Object : Private_Root) return Integer is
    begin
        return Object.Value;
    end Hidden;
end Prefixed_Model;
