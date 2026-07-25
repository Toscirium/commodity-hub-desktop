#pragma once

namespace Config {

// Same Supabase project used by the commodity-hub web app. The anon/publishable
// key is safe to embed client-side (it is subject to Postgres RLS policies),
// matching how the web client ships it in its bundle.
inline constexpr auto SupabaseUrl =
    "https://kcxhsmlqqyarhlmcapmj.supabase.co";
inline constexpr auto SupabaseAnonKey =
    "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9."
    "eyJpc3MiOiJzdXBhYmFzZSIsInJlZiI6ImtjeGhzbWxxcXlhcmhsbWNhcG1qIiwicm9sZSI6"
    "ImFub24iLCJpYXQiOjE3NDU3ODM0MDcsImV4cCI6MjA2MTM1OTQwN30."
    "qC25iAjNhbPVotryl7GONMgYkvg0DzEYp8uxioWzkfs";

inline constexpr auto OrganizationName = "CommodityHub";
inline constexpr auto ApplicationName = "CommodityHub Desktop";

} // namespace Config
