procedure Subprogramrenameerrors is
    function Original (X : Integer) return Integer is
    begin
        return X;
    end Original;
    procedure Action (X : in out Integer) is
    begin
        X := X + 1;
    end Action;
    function Wrong (X : Float) return Integer renames Original;
    procedure Wrong_Kind (X : Integer) renames Original;
    procedure Wrong_Mode (X : out Integer) renames Action;
    function Unknown (X : Integer) return Integer renames Missing;
    Value : Integer := 1;
    function Object_Target return Integer renames Value;
    function Original (X : Integer) return Integer renames Original;
    function Forward (X : Integer := 1) return Integer;
    function Forward (Other : Integer) return Integer renames Original;
    function Defaults (X : Integer := 1) return Integer;
    function Defaults (X : Integer := 2) return Integer renames Original;
    function Circular (X : Integer) return Integer;
    function Chain (X : Integer) return Integer renames Circular;
    function Circular (X : Integer) return Integer renames Chain;
    type Callback is access function (X : Integer) return Integer;
    type Callbacks is array (1 .. 2) of Callback;
    Items : Callbacks := (others => Original'Access);
    X : Integer := 1;
    function Parameter (X : Integer) return Integer renames Items(X).all;
    function Needs_Argument (X : Integer) return Integer renames Original;
    function Plus (L, R : Integer) return Integer renames "+";
    type Binary_Callback is access function (L, R : Integer) return Integer;
    F : Binary_Callback := Plus'Access;
begin
    Value := Needs_Argument;
end Subprogramrenameerrors;
