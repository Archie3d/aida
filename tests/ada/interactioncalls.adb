with Ada.Text_IO;
procedure InteractionCalls is
    type Vector is array (Integer range <>) of Integer;
    type Other is array (Integer range <>) of Integer;
    subtype Shifted is Other (10 .. 12);
    subtype Small is Integer range 0 .. 9;
    defaults : Integer := 0;
    selections : Integer := 0;
    entered : Integer := 0;
    function DefaultStep return Integer is
    begin
        defaults := defaults + 1;
        return 2;
    end DefaultStep;
    function First return Integer is
    begin
        selections := selections + 1;
        return 3;
    end First;
    generic
        type Item is range <>;
    procedure Change (value : in out Item; step : Item; fail : Boolean := False);
    procedure Change (value : in out Item; step : Item; fail : Boolean := False) is
    begin
        value := value + step;
        if fail then
            raise Constraint_Error;
        end if;
    end Change;
    procedure IntegerChange is new Change (Integer);
    procedure Update (target : in out Integer; amount : Integer := DefaultStep;
                      fail : Boolean := False) renames IntegerChange;
    procedure Update (target : in out Float; amount : Float := 1.0) is
    begin
        target := target + amount;
    end Update;
    procedure Modify (target : in out Shifted; fail : Boolean := False) is
    begin
        entered := entered + 1;
        Update (target => target (target'First));
        if fail then
            raise Constraint_Error;
        end if;
    end Modify;
    procedure Alias (items : in out Shifted; fail : Boolean := False) renames Modify;
    values : Vector (3 .. 5) := (1, 2, 3);
    smallValue : Small := 8;
    realValue : Float := 3.0;
    procedure Check (condition : Boolean) is
    begin
        if not condition then
            raise Program_Error;
        end if;
    end Check;
begin
    Alias (items => Shifted (values (First .. 5)));
    Check (values = (3, 2, 3) and selections = 1 and defaults = 1);
    begin
        Alias (fail => True, items => Shifted (values (First .. 5)));
        raise Program_Error;
    exception
        when Constraint_Error =>
            Check (values = (3, 2, 3) and selections = 2 and defaults = 2);
    end;
    begin
        Alias (items => Shifted (values (3 .. 4)));
        raise Program_Error;
    exception
        when Constraint_Error => Check (entered = 2 and defaults = 2);
    end;
    begin
        Update (target => smallValue);
        raise Program_Error;
    exception
        when Constraint_Error => Check (smallValue = 8 and defaults = 3);
    end;
    begin
        Update (fail => True, amount => 1, target => smallValue);
        raise Program_Error;
    exception
        when Constraint_Error => Check (smallValue = 8);
    end;
    Update (target => realValue);
    Check (realValue = 4.0 and defaults = 3);
    Ada.Text_IO.Put_Line ("interaction calls passed");
end InteractionCalls;
