with Ada.Text_IO;

procedure UnsignedArrayBounds is
    type Index is mod 2 ** 64;
    Midpoint : constant := 2 ** 63;
    Top : constant := 2 ** 64 - 1;
    type Buffer is array (Index range <>) of Character;
    type Signed_Buffer is array (Long_Integer range <>) of Character;
    type Words is array (Index range <>) of Long_Integer;
    type Table is array (Index range <>, Index range <>) of Character;
    type Buffer_Access is access Buffer;
    type Windowed (Start : Index) is record
        Data : Buffer (Start .. Midpoint + 1);
    end record;

    subtype Cross_Sign_Bit is Buffer (Midpoint - 1 .. Midpoint + 1);
    Static : Cross_Sign_Bit := ('a', 'b', 'c');
    Matrix : Table (Midpoint - 1 .. Midpoint, Top - 1 .. Top) :=
        ((Top - 1 => 'd', Top => 'e'), (Top - 1 => 'f', Top => 'g'));

    function Make (Low, High : Index) return Buffer is
        subtype Window is Buffer (Low .. High);
        Data : Window := (others => 'x');
    begin
        if Data'Length /= 0 then
            Data (High) := 'y';
        end if;
        return Data;
    end Make;

    function Identity (Value : Index) return Index is
    begin
        return Value;
    end Identity;

    procedure Check (Data : Buffer; Low, High : Index; Length : Natural) is
        Count : Natural := 0;
    begin
        if Data'First /= Low or else Data'Last /= High or else Data'Length /= Length then
            raise Program_Error;
        end if;
        for I in Data'Range loop
            if I /= Low + Index (Count) then
                raise Program_Error;
            end if;
            Count := Count + 1;
        end loop;
        if Count /= Length then
            raise Program_Error;
        end if;
    end Check;

    At_Top : Buffer := Make (Top - 2, Top);
    Empty : Buffer := Make (Top, 0);
    Low : Index := Identity (Midpoint - 1);
    High : Index := Identity (Midpoint + 1);
    Heap : Buffer_Access := new Buffer (Low .. High);
    Named : Buffer := (Top - 1 => 'm', Top => 'n');
    Signed_Value : Signed_Buffer (5 .. 6) := ('h', 'i');
    Window : Windowed (Midpoint - 1);
begin
    Check (Static, Midpoint - 1, Midpoint + 1, 3);
    Check (At_Top, Top - 2, Top, 3);
    Check (At_Top (Top - 1 .. Top), Top - 1, Top, 2);
    Check (At_Top (Top .. 0), Top, 0, 0);
    Check (Empty, Top, 0, 0);
    Check (Heap.all, Low, High, 3);
    Check (Named, Top - 1, Top, 2);
    Check (Empty & Named, Top - 1, Top, 2);
    Check (Buffer (Signed_Value), 5, 6, 2);
    Check (Buffer (Signed_Buffer (Buffer (Signed_Value))), 5, 6, 2);
    Window.Data := At_Top;
    Check (Window.Data, Midpoint - 1, Midpoint + 1, 3);
    if Cross_Sign_Bit'First /= Midpoint - 1 or else Cross_Sign_Bit'Last /= Midpoint + 1
        or else Static (Midpoint) /= 'b' or else At_Top (Top) /= 'y'
        or else Matrix (Midpoint, Top) /= 'g' or else Matrix'Last (2) /= Top
        or else Named (Top) /= 'n'
    then
        raise Program_Error;
    end if;

    Static := At_Top;
    if Static (Midpoint + 1) /= 'y' then
        raise Program_Error;
    end if;

    begin
        At_Top (Identity (0)) := 'z';
        raise Program_Error;
    exception
        when Constraint_Error => null;
    end;
    begin
        declare
            Invalid : Signed_Buffer := Signed_Buffer (At_Top);
        begin
            raise Program_Error;
        end;
    exception
        when Constraint_Error => null;
    end;
    begin
        Check (Make (Identity (0), Identity (Top)), 0, Top, 0);
        raise Program_Error;
    exception
        when Storage_Error => null;
    end;
    begin
        declare
            Too_Many_Bytes : Words (Identity (Top - 2 ** 62) .. Identity (Top));
        begin
            raise Program_Error;
        end;
    exception
        when Storage_Error => null;
    end;
    begin
        Check (Named & 'z', Top - 1, Top, 2);
        raise Program_Error;
    exception
        when Constraint_Error => null;
    end;
    Ada.Text_IO.Put_Line ("unsigned array bounds passed");
end UnsignedArrayBounds;
