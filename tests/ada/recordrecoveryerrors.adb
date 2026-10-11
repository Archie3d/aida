procedure RecordRecoveryErrors is
    type Node;
    type Node is record
        Next : access Node;
        Value : Integer;
        Other : access Node;
    end record;
    type Variant (Flag : Boolean) is record
        case Flag is
            when True =>
                Next : access Node;
            when False =>
                Value : Integer;
        end case;
    end record;
    type Missing_Component_Type is record
        Value :
    end record;
    After_Record : Integer := 0;
begin
    After_Record := 1;
    goto Destination;
end RecordRecoveryErrors;
