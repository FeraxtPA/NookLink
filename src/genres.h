#pragma once

#include <unordered_map>
#include <string>
#include "../include/nlohmann/json.hpp"

//Might need to use strings later so that user can input custom genres
enum class Genre {
	Dystopian,
	ScienceFiction,
	Fantasy,
	Adventure,
	History,
	LiteraryFiction,
	HistoricalFiction,
	Mystery,
	Thriller,
	Romance,
	Horror,
	YoungAdult,
	ChildrensLiterature,
	GraphicNovel,
	MagicalRealism,
	ChickLit,
	NewAdult,
	PostApocalyptic,
	Western,
	CrimeFiction,
	LiteraryMystery,
	Biography,
	Memoir,
	SelfHelp,
	TrueCrime,
	TravelWriting,
	Cookbook,
	Essay,
	Science,
	Politics,
	Philosophy,
	HealthAndWellness,
	Business,
	Parenting,
	NatureWriting,
	Spirituality,
	Sociology,
	Psychology,
	Education,
	Finance,
	FanFiction,
	ClassicLiterature,
	Romanticism,
	Realism,
	Modernism,
	Surrealism,
	GothicFiction,
	VictorianLiterature,
	BeatGeneration,
	Existentialism,
	HistoricalRomance,
	CozyMystery,
	UrbanFantasy,
	HorrorComedy,
	ShortStories,
	Poetry,
	Journals,
	Action,
	DarkFantasy,
	CozyFantasy,
	Nature

};

inline std::string genreToString(Genre genre) {
    static const std::unordered_map<Genre, std::string> genreMap = {
        {Genre::Dystopian, "Dystopian"},
        {Genre::ScienceFiction, "Science Fiction"},
        {Genre::Fantasy, "Fantasy"},
        {Genre::Adventure, "Adventure"},
        {Genre::History, "History"},
        {Genre::LiteraryFiction, "Literary Fiction"},
        {Genre::HistoricalFiction, "Historical Fiction"},
        {Genre::Mystery, "Mystery"},
        {Genre::Thriller, "Thriller"},
        {Genre::Romance, "Romance"},
        {Genre::Horror, "Horror"},
        {Genre::YoungAdult, "Young Adult"},
        {Genre::ChildrensLiterature, "Children's Literature"},
        {Genre::GraphicNovel, "Graphic Novel"},
        {Genre::MagicalRealism, "Magical Realism"},
        {Genre::ChickLit, "Chick Lit"},
        {Genre::NewAdult, "New Adult"},
        {Genre::PostApocalyptic, "Post Apocalyptic"},
        {Genre::Western, "Western"},
        {Genre::CrimeFiction, "Crime Fiction"},
        {Genre::LiteraryMystery, "Literary Mystery"},
        {Genre::Biography, "Biography"},
        {Genre::Memoir, "Memoir"},
        {Genre::SelfHelp, "Self Help"},
        {Genre::TrueCrime, "True Crime"},
        {Genre::TravelWriting, "Travel Writing"},
        {Genre::Cookbook, "Cookbook"},
        {Genre::Essay, "Essay"},
        {Genre::Science, "Science"},
        {Genre::Politics, "Politics"},
        {Genre::Philosophy, "Philosophy"},
        {Genre::HealthAndWellness, "Health and Wellness"},
        {Genre::Business, "Business"},
        {Genre::Parenting, "Parenting"},
        {Genre::NatureWriting, "Nature Writing"},
        {Genre::Spirituality, "Spirituality"},
        {Genre::Sociology, "Sociology"},
        {Genre::Psychology, "Psychology"},
        {Genre::Education, "Education"},
        {Genre::Finance, "Finance"},
        {Genre::FanFiction, "Fan Fiction"},
        {Genre::ClassicLiterature, "Classic Literature"},
        {Genre::Romanticism, "Romanticism"},
        {Genre::Realism, "Realism"},
        {Genre::Modernism, "Modernism"},
        {Genre::Surrealism, "Surrealism"},
        {Genre::GothicFiction, "Gothic Fiction"},
        {Genre::VictorianLiterature, "Victorian Literature"},
        {Genre::BeatGeneration, "Beat Generation"},
        {Genre::Existentialism, "Existentialism"},
        {Genre::HistoricalRomance, "Historical Romance"},
        {Genre::CozyMystery, "Cozy Mystery"},
        {Genre::UrbanFantasy, "Urban Fantasy"},
        {Genre::HorrorComedy, "Horror Comedy"},
        {Genre::ShortStories, "Short Stories"},
        {Genre::Poetry, "Poetry"},
        {Genre::Journals, "Journals"},
        {Genre::Action, "Action"},
        {Genre::DarkFantasy, "Dark Fantasy"},
        {Genre::CozyFantasy, "Cozy Fantasy"},
        {Genre::Nature, "Nature"}
    };

    auto it = genreMap.find(genre);
    if (it != genreMap.end()) {
        return it->second;
    }
    return "Unknown Genre";
}

// === ADD THIS FUNCTION ===
inline Genre stringToGenre(const std::string& genreStr) {
    // Create a reverse map for efficient lookup
    static const std::unordered_map<std::string, Genre> reverseGenreMap = {
        {"Dystopian", Genre::Dystopian},
        {"Science Fiction", Genre::ScienceFiction},
        {"Fantasy", Genre::Fantasy},
        {"Adventure", Genre::Adventure},
        {"History", Genre::History},
        {"Literary Fiction", Genre::LiteraryFiction},
        {"Historical Fiction", Genre::HistoricalFiction},
        {"Mystery", Genre::Mystery},
        {"Thriller", Genre::Thriller},
        {"Romance", Genre::Romance},
        {"Horror", Genre::Horror},
        {"Young Adult", Genre::YoungAdult},
        {"Children's Literature", Genre::ChildrensLiterature},
        {"Graphic Novel", Genre::GraphicNovel},
        {"Magical Realism", Genre::MagicalRealism},
        {"Chick Lit", Genre::ChickLit},
        {"New Adult", Genre::NewAdult},
        {"Post Apocalyptic", Genre::PostApocalyptic},
        {"Western", Genre::Western},
        {"Crime Fiction", Genre::CrimeFiction},
        {"Literary Mystery", Genre::LiteraryMystery}, 
        {"Biography", Genre::Biography},
        {"Memoir", Genre::Memoir},
        {"Self Help", Genre::SelfHelp},
        {"True Crime", Genre::TrueCrime},
        {"Travel Writing", Genre::TravelWriting},
        {"Cookbook", Genre::Cookbook},
        {"Essay", Genre::Essay},
        {"Science", Genre::Science},
        {"Politics", Genre::Politics},
        {"Philosophy", Genre::Philosophy},
        {"Health and Wellness", Genre::HealthAndWellness},
        {"Business", Genre::Business},
        {"Parenting", Genre::Parenting},
        {"Nature Writing", Genre::NatureWriting},
        {"Spirituality", Genre::Spirituality},
        {"Sociology", Genre::Sociology},
        {"Psychology", Genre::Psychology},
        {"Education", Genre::Education},
        {"Finance", Genre::Finance},
        {"Fan Fiction", Genre::FanFiction},
        {"Classic Literature", Genre::ClassicLiterature},
        {"Romanticism", Genre::Romanticism},
        {"Realism", Genre::Realism},
        {"Modernism", Genre::Modernism},
        {"Surrealism", Genre::Surrealism},
        {"Gothic Fiction", Genre::GothicFiction},
        {"Victorian Literature", Genre::VictorianLiterature},
        {"Beat Generation", Genre::BeatGeneration},
        {"Existentialism", Genre::Existentialism},
        {"Historical Romance", Genre::HistoricalRomance},
        {"Cozy Mystery", Genre::CozyMystery},
        {"Urban Fantasy", Genre::UrbanFantasy},
        {"Horror Comedy", Genre::HorrorComedy},
        {"Short Stories", Genre::ShortStories},
        {"Poetry", Genre::Poetry},
        {"Journals", Genre::Journals},
        {"Action", Genre::Action},
        {"Dark Fantasy", Genre::DarkFantasy},
        {"Cozy Fantasy", Genre::CozyFantasy},
        {"Nature", Genre::Nature}
    };

    auto it = reverseGenreMap.find(genreStr);
    if (it != reverseGenreMap.end()) {
        return it->second;
    }
    throw std::runtime_error("Unknown Genre: " + genreStr);
    }


// Overload to convert enum to json string 
inline void to_json(nlohmann::json& j, const Genre& g)
{
    j = genreToString(g);
}
// Overload to convert json string to enum
inline void from_json(const nlohmann::json& j, Genre& g)
{
    g = stringToGenre(j.get<std::string>());
}


