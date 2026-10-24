with Prefixed_Model;
procedure PrefixedErrors is
    X : Prefixed_Model.Root;
    H : Prefixed_Model.Private_Root;
    C : constant Prefixed_Model.Root := (Value => 1);
    Y : aliased Prefixed_Model.Child;
    type Constant_Pointer is access constant Prefixed_Model.Child;
    P : Constant_Pointer := Y'Access;
    procedure Outside (Object : Prefixed_Model.Root) is
    begin
        null;
    end Outside;
    function Make return Prefixed_Model.Root is
    begin
        return X;
    end Make;
    function Choice return Prefixed_Model.Root is
    begin
        return X;
    end Choice;
    type Pointer is access all Prefixed_Model.Root;
    function Choice return Pointer is
    begin
        return null;
    end Choice;
    type Plain is record
        Value : Integer := 0;
    end record;
    procedure Invalid (Object : Plain) is
    begin
        null;
    end Invalid;
    Untagged : Plain;
    Number : Integer;
begin
    C.Add;
    X.Set (1);
    P.Set (1);
    Make.Add;
    Number := H.Hidden;
    X.Outside;
    X.Add (Object => X);
    X.Add (1, Amount => 2);
    X.Add (Amount => 1, 2);
    X.Add (Amount => 1, Amount => 2);
    Number := X.Read (1);
    Number := Make.Via_Access;
    Number := Choice.Read;
    X.Named_First;
    X.Second (1);
    Untagged.Invalid;
end PrefixedErrors;
