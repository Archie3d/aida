pragma Assertion_Policy (Unknown);
pragma Suppress (Bogus_Check);
pragma Unsuppress (Unknown_Check);
procedure PragmaPlacementErrors is
    pragma Assertion_Policy (Check);
    pragma List (Maybe);
    pragma Optimize (Fast);
    pragma Page (1);
begin
    pragma Suppress (Range_Check);
    pragma Unsuppress (All_Checks);
    pragma No_Return (PragmaPlacementErrors);
    pragma Controlled (Pointer);
end PragmaPlacementErrors;
pragma Assert (True);
