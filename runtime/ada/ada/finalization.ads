package Ada.Finalization is
    pragma Preelaborate;
    type Controlled is abstract tagged private;
    pragma Preelaborable_Initialization (Controlled);
    procedure Initialize (Object : in out Controlled) is null;
    procedure Adjust (Object : in out Controlled) is null;
    procedure Finalize (Object : in out Controlled) is null;

    type Limited_Controlled is abstract tagged limited private;
    pragma Preelaborable_Initialization (Limited_Controlled);
    procedure Initialize (Object : in out Limited_Controlled) is null;
    procedure Finalize (Object : in out Limited_Controlled) is null;
private
    type Controlled is abstract tagged null record;
    type Limited_Controlled is abstract tagged limited null record;
end Ada.Finalization;
