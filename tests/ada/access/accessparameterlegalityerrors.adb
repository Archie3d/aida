procedure AccessParameterLegalityErrors is
    generic
        type Item is private;
    procedure Template (Self : access Item);
    procedure Template (Self : access Item) is
    begin
        null;
    end Template;
    package Model is
        type Root is tagged null record;
        package Source is
            procedure Nullable (Self : access Root) is null;
        end Source;
        procedure Renamed (Self : access Root) renames Source.Nullable;
        procedure Body_Rename (Self : access Root);
        procedure Instance is new Template (Root);
    end Model;
    package body Model is
        procedure Body_Rename (Self : access Root) renames Source.Nullable;
    end Model;
begin
    null;
end AccessParameterLegalityErrors;
