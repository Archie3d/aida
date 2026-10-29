package body Ada.Environment_Variables is
    function Count return Natural;
    pragma Import (C, Count, "__ada_host_env_count");
    function Entry_Name (Index : Natural) return String;
    pragma Import (C, Entry_Name, "__ada_host_env_name");
    function Entry_Value (Index : Natural) return String;
    pragma Import (C, Entry_Value, "__ada_host_env_entry_value");
    procedure Iterate (Process : not null access procedure (Name, Value : String)) is
        Length : constant Natural := Count;
    begin
        for Index in 1 .. Length loop
            Process (Entry_Name (Index - 1), Entry_Value (Index - 1));
        end loop;
    end Iterate;
end Ada.Environment_Variables;
