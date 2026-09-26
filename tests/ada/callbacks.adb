with Ada.Text_IO; use Ada.Text_IO;

procedure Callbacks is
    type Callback_Type is access function (X : Integer) return Integer;
    type Action_Type is access procedure (X : in out Integer);
    type Getter_Type is access function return Integer;
    type Notice_Type is access procedure;
    type Text_Type is access function (X : Integer) return String;
    Offset : Integer := 10;

    function Add (X : Integer) return Integer is
    begin
        return X + Offset;
    end Add;

    function Add (X : Float) return Float is
    begin
        return X + 1.0;
    end Add;

    procedure Increment (X : in out Integer) is
    begin
        X := X + Offset;
    end Increment;

    function Get return Integer is
    begin
        return Offset;
    end Get;

    procedure Notice is
    begin
        Offset := Offset + 1;
    end Notice;

    function Text (X : Integer) return String is
    begin
        return Integer'Image (X + Offset);
    end Text;

    function Apply (F : Callback_Type; X : Integer) return Integer is
    begin
        return F (X);
    end Apply;

    -- Taking 'Access in a helper must not tie the value to the helper's stack.
    function Choose return Callback_Type is
    begin
        return Add'Access;
    end Choose;

    F : Callback_Type := Choose;
    P : Action_Type := Increment'Access;
    G : Getter_Type := Get'Access;
    N : Notice_Type := Notice'Access;
    T : Text_Type := Text'Access;
    Empty : Callback_Type;
    Value : Integer := 2;
    type Holder is record
        Callback : Callback_Type;
    end record;
    H : Holder := (Callback => Add'Access);
    type Callback_Array is array (1 .. 2) of Callback_Type;
    Items : Callback_Array := (Add'Access, null);
begin
    Put_Line (Integer'Image (F (2)));
    Put_Line (Integer'Image (F.all (X => 3)));
    Put_Line (Integer'Image (Apply (Add'Access, 4)));
    P (Value);
    Put_Line (Integer'Image (Value));
    N;
    N.all;
    Put_Line (Integer'Image (G.all));
    Put_Line (T (1));
    Put_Line (Integer'Image (H.Callback (2)));
    Put_Line (Integer'Image (Items (1) (3)));
    if F = Add'Access and Empty = null then
        Put_Line ("identity and null");
    end if;
    F := null;
    begin
        Value := F (1);
    exception
        when Constraint_Error => Put_Line ("null callback");
    end;
    F := Callback_Type (Choose);
    Offset := 20;
    Put_Line (Integer'Image (F (1)));
end Callbacks;
