with Ada.Text_IO;
procedure ComponentEquality is
    Calls : Integer := 0;
    Plain_Calls : Integer := 0;
    Failure : exception;
    package Model is
        type Tagged_Item is tagged record
            Value : Integer;
        end record;
        overriding function "=" (Left, Right : Tagged_Item) return Boolean;
        type Plain_Item is record
            Value : Integer;
        end record;
        function "=" (Left, Right : Plain_Item) return Boolean;
    end Model;
    package body Model is
        function "=" (Left, Right : Tagged_Item) return Boolean is
        begin
            Calls := Calls + 1;
            if Left.Value < 0 or Right.Value < 0 then raise Failure; end if;
            return Left.Value mod 10 = Right.Value mod 10;
        end "=";
        function "=" (Left, Right : Plain_Item) return Boolean is
        begin
            Plain_Calls := Plain_Calls + 1;
            raise Failure;
            return False;
        end "=";
    end Model;
    type Items is array (Integer range <>) of Model.Tagged_Item;
    type Grid is array (1 .. 2, 1 .. 2) of Model.Tagged_Item;
    type Holder is record
        Item : Model.Tagged_Item;
    end record;
    type Nested is record
        Item : Holder;
    end record;
    type Plain_Holder is record
        Item : Model.Plain_Item;
    end record;
    type Tagged_Holder is tagged record
        Item : Model.Plain_Item;
    end record;
    type Plain_Items is array (1 .. 2) of Model.Plain_Item;
    generic
        type T is private;
    function Same (Left, Right : T) return Boolean;
    function Same (Left, Right : T) return Boolean is
    begin
        return Left = Right;
    end Same;
    function Same_Holder is new Same (Holder);
    function Same_Plain is new Same (Model.Plain_Item);
    A : Items (3 .. 4) := ((Value => 1), (Value => 2));
    B : Items (-2 .. -1) := ((Value => 11), (Value => 22));
    L : Grid := (((Value => 1), (Value => 2)), ((Value => 3), (Value => 4)));
    R : Grid := (((Value => 11), (Value => 12)), ((Value => 13), (Value => 14)));
    H : Holder := (Item => (Value => 1));
    J : Holder := (Item => (Value => 21));
    N : Nested := (Item => H);
    M : Nested := (Item => J);
    P : Plain_Holder := (Item => (Value => -1));
    Q : Plain_Holder := P;
    T : Tagged_Holder := (Item => (Value => -1));
    U : Tagged_Holder := T;
    X : Plain_Items := (others => (Value => -1));
    Y : Plain_Items := X;
    procedure Check (Condition : Boolean) is
    begin
        if not Condition then raise Program_Error; end if;
    end Check;
begin
    Check (A = B and not (A /= B));
    Check (Calls = 4);
    Check (L = R);
    Check (Calls = 8);
    Check (H = J and N = M and Same_Holder (H, J));
    Check (Calls = 11);
    Check (P = Q and T = U and X = Y);
    Check (Same_Plain ((Value => -1), (Value => -1)));
    Check (Plain_Calls = 0);
    B (-1).Value := 23;
    Check (A /= B);
    B (-2).Value := -1;
    begin
        if A = B then raise Program_Error; end if;
        raise Program_Error;
    exception
        when Failure => null;
    end;
    B (-2).Value := 11;
    B (-1).Value := 22;
    Check (A = B);
    Ada.Text_IO.Put_Line ("component equality passed");
end ComponentEquality;
