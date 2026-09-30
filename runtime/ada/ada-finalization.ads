package Ada.Finalization is
    type Controlled is abstract tagged private;
    procedure Initialize (Object : in out Controlled) is null;
    procedure Adjust (Object : in out Controlled) is null;
    procedure Finalize (Object : in out Controlled) is null;

    type Limited_Controlled is abstract tagged limited private;
    procedure Initialize (Object : in out Limited_Controlled) is null;
    procedure Finalize (Object : in out Limited_Controlled) is null;
private
    type Controlled is abstract tagged null record;
    type Limited_Controlled is abstract tagged limited null record;
end Ada.Finalization;
