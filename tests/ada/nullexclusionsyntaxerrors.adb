procedure NullExclusionSyntaxErrors is
    type Link is access Integer;
    P : access not null Link;
begin
    null;
end NullExclusionSyntaxErrors;
