You are developing a fantasy RPG currently centered around the working title The Hollow King.

The world’s ancient history revolves around the Forge Kingdom, once the dominant power across much of the known world. In 903 After The Visit, it fought the Six Day Battle against a much smaller but vastly more powerful force led by Gudloth, a rogue wizard from the Enchanted Lands. Gudloth was the son of the ancient mother of those lands, spent decades practicing magic in the Old Woods, reached demigod-like power, gathered followers, and eventually crushed the Forge Kingdom’s combined dwarven, elven, and magical armies. The surviving Forge forces withdrew beyond the Rocky Cliffs and built an enormous barrier.

Afterward, Gudloth founded La Poise near the delta of the Lace River. The collapse of the Forge Kingdom created a power vacuum: dwarves withdrew toward the mountains, elves toward their swamp, ordinary magicians scattered, while Gudloth’s especially powerful apprentices remained important figures. La Poise became the emerging dominant civilization.

The game initially presents a different version of this history. The player believes they are the spirit/demon of the defeated ruler associated with the Forge Kingdom, returned to seek revenge against Gudloth. The apparent overarching objective is therefore to grow stronger and ultimately kill Gudloth/the ruler protected in the northeastern old city.

The central late-game twist is that the player character is actually Gudloth. Gudloth has returned in a diminished, fragmented or amnesiac spiritual form, while the person the player has been treating as the villain is actually the good/rightful king. The player has therefore unknowingly been carrying out the return of the ancient villain. You want earlier dialogue, lore and presentation to remain technically compatible with this truth so that the story gains a second meaning after the reveal rather than relying on an outright contradiction.

The modern world has several major factions. Dwarves have recently arrived by sea and established a colony in the mountains, living in small green houses while mining valuable metals and searching for some legendary object, potentially something such as a dragon egg. Wizards inhabit the east, remain mysterious, and are trying to destroy the demon/player character. They also protect the old city and its rightful king. Knights protect the main city and are fighting for fertile land and resources because rapid population growth has created food shortages. Archers live nomadically in the forest/jungle, strongly identify with nature and despise the city’s expansion and deforestation. They attack city expeditions and use dead enemies as fertilizer when restoring forest destroyed for agriculture and colonies.

A significant gameplay/narrative mechanic is that the demon inhabits or switches between different characters/classes. One planned transition occurs when forest archers ambush a city expedition, killing and capturing knights; the demon then transfers from the knight character to an archer. This lets the player experience competing factions from inside rather than simply treating one side as objectively good.

Other established locations include:

Elf Swamp: home of roughly 0.7 m tall elves. They are skilled archers, historically served the Forge Kingdom and now raid settlements and clash with La Poise.
La Poise: bounded largely by the Lace River and Kirabata, with an unstable eastern frontier against the elves. A defensive wall is being constructed there.
Fort Delta / Fort Dom: La Poise military outposts.
Old Woods: ancient forest containing creatures, treasures and important historical/magical significance.
Enchanted Lands: Gudloth’s place of origin and an important magical region.
Rock Plains / Rocky Cliffs: site and surroundings of the ancient Forge Kingdom defeat.
Old City in the northeast: contains the rightful king and is protected by the wizards.

You also established altars as important gameplay points where the player can heal, save and switch between available classes/forms.

For the title, we explored names such as Ashes of the Forge, The Sixth Day, The Forgotten Name, Crownless, The Flame Remains, Revenant of the Sixth Day, and The Hollow King. The strongest direction became The Hollow King because it can have two interpretations:

Before the reveal, the player can interpret the “Hollow King” as the dead/defeated ruler whose spirit they believe themselves to be.
After the reveal, “Hollow” can refer to Gudloth himself existing as an incomplete vessel, spirit, forgotten identity or empty version of his former demigod self. “King” can simultaneously be intentionally misleading: the player assumes they are reclaiming something that belonged to them, while the real rightful king is the person they are moving toward killing.

The design goal for the title is therefore retrospective meaning: after learning the truth, the player should realize that the title was describing their actual identity or condition all along.

We discussed subtle foreshadowing that could support this. Particularly suitable ideas were recurring imagery of an empty/broken crown, references to someone having forgotten their name, and lines whose apparent meaning changes after the reveal—for example the concept that a god can forget himself while wearing a hollow crown. Save altars and ancient murals could similarly describe the returning entity without explicitly naming Gudloth.

A logo for The Hollow King was then generated. It used a dark background, pale distressed medieval lettering and a crown motif, with a concealed/stylized G incorporated around the word KING as a visual reference to Gudloth. The intention was for this to look like ordinary decorative typography initially but become recognizable as deliberate foreshadowing after the reveal.

Separately, several C++ files from your engine are now available in this conversation. They include the current StoryManager, event system, quest structures, quadtree and level loader. Your existing story architecture is already event-driven: quests contain required event types/subjects, StoryManager loads them from JSON and reacts to events such as item pickup, entity death, dialogue completion or flag changes. EventBus routes events by itemName, while QuestStep matches both event type and subject.

That architecture is directly relevant to the lore direction because the Gudloth reveal, faction-state changes, body/class switching, misleading historical knowledge and later reinterpretation can eventually be represented through story flags and event-driven quest progression rather than hard-coded scene logic.

Titles we discussed:

Ashes of the Forge
The Forged Revenant
Echoes of the Anvil
Ironbound
Beneath the Forge
The Sixth Day
Gudloth’s Wake
Son of the Mother
The Demigod War
Twilight of the Forge
Vengeance of the Hollow King
Spirit of Iron
The Black Gate Rises
To Kill a God
Break the Barrier
La Poise Must Fall
Rise Beyond the Cliffs
The Rock Plains Remember
Kirabata Frontier
The Cursed Sea and the Lace River
Gudloth
903 A.V.
Anvil
The Visit
Fracture
Wrought by Flame
The Last Forge
The Revenant Crown
Thronebreaker
Av Grusets Rike
Kingdoms of Ash
Bloodwood
Iron and Root
The Warring Boughs
Gudloth’s Peace
Echoes of The Visit
Whispers Beyond the Barrier
Vapors of the Enchanted Lands
The Last Demon
He Who Crawls
Curseborn
Fallen Flame
Children of the Gate
Smedjans Skugga
Den Sista Dagen
Järnets Vrede
Förgätna Vägar
Kungamördaren
The Hollow King
Revenant of the Sixth Day
Shadow of the Gate
Ash of Gudloth
Crownless
Throne Reclaimed
The Flame Remains
Of Ash and Godblood
The Forgotten Name
What the River Stole
The Just War
Eidolon
Palingenesis
Ruinbearer

We also discussed these title/subtitle combinations:

The Revenant Crown: Book of Ash and Iron
X: Curse of the Forged Lands
Ironbound: Rise of the Hollow King
Gudloth’s Fall: A Demon’s Tale
The Hollow King: A Crown Returns
Revenant of the Sixth Day: The God in Disguise
Ash of Gudloth: The Forgotten Name
Throne Reclaimed: A Just War

The title we developed furthest was The Hollow King.