procedure Unhandledmessage is
begin
    raise Program_Error with "visible failure";
exception
    when others => raise;
end Unhandledmessage;
