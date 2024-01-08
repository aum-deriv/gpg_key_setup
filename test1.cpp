/*
	The following script generates a GPG key
*/

#include <iostream>
#include <regex>
using namespace std;

vector<string> split(string s, regex r)
{

	vector<string> splits;
	smatch m; // <-- need a match object

	while (regex_search(s, m, r)) // <-- use it here to get the match
	{
		int split_on = m.position(); // <-- use the match position
		splits.push_back(s.substr(0, split_on));
		s = s.substr(split_on + m.length()); // <-- also, skip the whole match
	}

	if (!s.empty())
	{
		splits.push_back(s); // and there may be one last token at the end
	}

	return splits;
}

std::string exec(const char *cmd)
{
	// [2] copy-pasta ;)

	std::array<char, 128> buffer;
	std::string result;
	std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(cmd, "r"), pclose);
	if (!pipe)
	{
		throw std::runtime_error("popen() failed!");
	}
	while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr)
	{
		result += buffer.data();
	}
	return result;
}

int generatePrivateKey()
{
	const char *command_addGpgConfig = "#!/bin/bash \
		cat >gpg_config <<EOF \
		%echo Generating a basic OpenPGP key \
		Key-Type: RSA \
		Key-Length: 4096 \
		Name-Real: aum-test \
		Name-Email: aum@foo.bar \
		Expire-Date: 0 \
		%commit \
		%echo done \
	EOF";

	const char *command_generatePrivateKey = "gpg --batch --passphrase='' --full-generate-key gpg_config";

	const char *command_removeGpgConfig = "rm -rf ./gpg_config";

	exec(command_addGpgConfig);		  // add gpg_config file
	exec(command_generatePrivateKey); // generate private key
	exec(command_removeGpgConfig);	  // remove gpg_config file
}

std::string generatePublicKey()
{
	std::string pattern("rsa4096/");
	std::regex rx(pattern);

	std::string keys = exec("gpg --list-secret-keys --keyid-format=long");

	auto rsa4096Split = split(keys, rx);					   // split at rsa4096
	auto spaceSplit = split(rsa4096Split[1], std::regex(" ")); // split the 2nd element at blank-spaces
	std::string keyId = spaceSplit.front();					   // extract the keyId

	// run command to generate the public key
	const char *command_generatePubKey = ("gpg --armor --export " + keyId).c_str();
	std::string pubKey = exec(command_generatePubKey);

	// copy the public key to clipboard
	const char *command_copyPubKey = ("echo \"" + pubKey + "\" | pbcopy").c_str();
	exec(pubKey.c_str());

	return keyId.c_str();
}

int configGitAutoSigning(std::string keyId)
{
	// unset the current signing key (if any)
	exec("git config --global --unset gpg.format");

	// set the new signing key
	exec(("git config --global user.signingkey " + keyId).c_str());
	exec(("git config --global user.signingkey " + keyId + '!').c_str());

	exec("if [ -r ~/.zshrc ]; then echo -e '\nexport GPG_TTY=$(tty)' >> ~/.zshrc; \
  else echo -e '\nexport GPG_TTY=$(tty)' >> ~/.zprofile; fi");
	exec("if [ -r ~/.bash_profile ]; then echo -e '\nexport GPG_TTY=$(tty)' >> ~/.bash_profile; \
  else echo -e '\nexport GPG_TTY=$(tty)' >> ~/.profile; fi");

	exec("killall gpg-agent");

	// [3] fix for auto-signing
	exec("git config --global gpg.program \"$(which gpg)\"");
	exec("echo \"no-tty\" >> ~/.gnupg/gpg.conf");
}

int reinstallGpg()
{
	const char *command_uninstallGpg = "brew uninstall gnupg";
	const char *command_removeGpgConfigs = "rm -rf ~/.gnupg";
	const char *command_installGpg = "brew install gnupg";

	exec(command_uninstallGpg);
	exec(command_removeGpgConfigs);
	exec(command_installGpg);
}

int main()
{

	reinstallGpg();
	std::string keyId = generatePublicKey();
	configGitAutoSigning(keyId);

	return 0;
}

/*
	References:
	[2] https://stackoverflow.com/questions/478898/how-do-i-execute-a-command-and-get-the-output-of-the-command-within-c-using-po
	[3] https://stackoverflow.com/questions/36941533/git-hub-desktop-on-mac-error-cannot-run-gpg-no-such-file-or-directory
*/
