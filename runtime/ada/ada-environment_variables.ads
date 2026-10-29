package Ada.Environment_Variables is
    pragma Preelaborate (Environment_Variables);
    function Value (Name : String) return String;
    pragma Import (C, Value, "__ada_host_env_value");
    function Exists (Name : String) return Boolean;
    pragma Import (C, Exists, "__ada_host_env_exists");
    procedure Set (Name, Value : String);
    pragma Import (C, Set, "__ada_host_env_set");
    procedure Clear (Name : String);
    pragma Import (C, Clear, "__ada_host_env_clear");
    procedure Clear;
    pragma Import (C, Clear, "__ada_host_env_clear_all");
    procedure Iterate (Process : not null access procedure (Name, Value : String));
end Ada.Environment_Variables;
