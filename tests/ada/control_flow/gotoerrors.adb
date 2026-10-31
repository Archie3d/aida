procedure GotoErrors is
    Value : Integer := 0;
    procedure Nested is
    begin
        goto Outer_Label;
    end Nested;
begin
    goto Missing;
    goto Value;
    goto Named_Loop;
    goto In_Block;
    goto In_Loop;
    goto In_Then;
    goto In_Handler;
    Named_Loop : loop
        <<In_Loop>> null;
        exit;
    end loop Named_Loop;
    begin
        <<In_Block>> null;
    end;
    if True then
        <<In_Then>> null;
    else
        goto In_Then;
    end if;
    case Value is
        when 0 =>
            <<In_Case>> null;
        when others => goto In_Case;
    end case;
    begin
        <<Protected_Label>> null;
    exception
        when Constraint_Error =>
            <<In_Handler>>
            goto Protected_Label;
        when others => goto In_Handler;
    end;
    declare
        procedure Inner_Body is
        begin
            goto GotoErrors.Outer_Label;
        end Inner_Body;
        GotoErrors : Integer := 0;
    begin
        goto GotoErrors.Outer_Label;
        Value := Outer_Label;
        Outer_Label := 1;
    end;
    declare
        procedure GotoErrors is
        begin
            null;
        end GotoErrors;
    begin
        goto GotoErrors.Outer_Label;
    end;
    <<Outer_Label>> null;
    <<Duplicate>> null;
    begin
        <<dUPLICATE>> null;
    end;
    <<Value>> null;
    Collision : loop
        exit;
    end loop Collision;
    <<Collision>> null;
end GotoErrors;
