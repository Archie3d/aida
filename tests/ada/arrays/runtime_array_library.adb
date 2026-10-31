package body Runtime_Array_Library is
    Calls : Integer := 0;
    function Bound return Integer is
    begin
        Calls := Calls + 1;
        return N + Calls;
    end Bound;
    subtype Body_Text is String (1 .. Bound);
    First, Second : String (1 .. Bound) := (others => 'x');
    Body_Value : Body_Text := (others => 'y');
    function Make return Text is
    begin
        return (11 .. 12 => 'a');
    end Make;
    procedure Inspect (X : Text; Y : Shape) is
        Copy : Alias_Text := X;
        Later : Shape := Y;
    begin
        if Copy /= A or Later /= M or X'First /= 2 or Y'First (2) /= 5 then
            raise Program_Error;
        end if;
    end Inspect;
    function Make_Hidden return Hidden is
    begin
        return H;
    end Make_Hidden;
    procedure Check_Hidden (X : Hidden) is
    begin
        if X /= H or X'Length /= 3 then raise Program_Error; end if;
    end Check_Hidden;
    package body Capture is
        procedure Check is
        begin
            if X'First /= 2 or X'Last /= 3 or X /= "aa"
                or Snapshot /= "aa" or Y'First /= 4 or Y'Last /= 5 then
                raise Program_Error;
            end if;
            Y (4) := 'z';
        end Check;
    end Capture;
begin
    N := 100;
    if Calls /= 3 or First'Length /= 5 or Second'Length /= 6
        or Body_Text'Length /= 4 or Body_Value'Length /= 4 then
        raise Program_Error;
    end if;
end Runtime_Array_Library;
