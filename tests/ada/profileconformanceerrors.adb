procedure ProfileConformanceErrors is
    subtype Small is Integer range 1 .. 10;
    subtype Smaller is Integer range 1 .. 5;
    type Vector is array (Integer range <>) of Integer;
    subtype Two is Vector (1 .. 2);
    subtype Three is Vector (1 .. 3);
    procedure Names (Value : Integer);
    procedure Names (Other : Integer) is null;
    procedure Modes (Value : in out Integer);
    procedure Modes (Value : out Integer) is null;
    procedure Bounds (Value : Small);
    procedure Bounds (Value : Smaller) is null;
    procedure Arrays (Value : Two);
    procedure Arrays (Value : Three) is null;
    function Result return Small;
    function Result return Smaller is begin return 1; end Result;
    procedure Defaults (Value : Integer := 1);
    procedure Defaults (Value : Integer := 2) is null;
    procedure New_Default (Value : Integer);
    procedure New_Default (Value : Integer := 1) is null;
    package Values is
        Value : constant := 1;
    end Values;
    use Values;
    procedure Binding (Value : Integer := Values.Value);
    Value : constant := 1;
    procedure Binding (Value : Integer := Value) is null;
begin
    null;
end ProfileConformanceErrors;
