<?php

require_once("config.inc.php");

setlocale(LC_ALL, "en_US.UTF-8");

$parser = new news_parser(dirname(__FILE__) . "/NEWS");
$releases = $parser->get_releases(10);

// ----------------------------------------------------------------------------

class news_parser
{
    private $fp, $sections;

    public function __construct($file)
    {
        $this->fp = fopen($file, 'r');
        if (!$this->fp)
            throw Exception("couldn't open '$file' for reading");

        $this->sections = array(
            "changes", "new features", "bugs fixed", "other", "internal",
            "security related changes", "other changes"
        );
    }

    public function __destruct()
    {
        fclose($this->fp);
    }

    public function get_releases($count)
    {
        $releases = array();
        for ($i=0; $i<$count; ++$i)
        {
            $release = $this->get_release();
            if ($release === false)
                break;
            $releases[] = $release;
        }
        return $releases;
    }

    public function get_release()
    {
        $timestamp = $this->get_timestamp();
        if ($timestamp === false)
            return false;

        return array(
            "timestamp" => $timestamp,
            "header"    => $this->get_header(),
            "sections"  => $this->get_sections()
        );
    }

    public function get_timestamp()
    {
        $this->eat_whitespace();
        $date = $this->get_line();
        if ($date === false)
            return false;
        return strtotime($date);
    }

    public function get_header()
    {
        $this->eat_whitespace();
        return trim($this->get_line());
    }

    public function get_sections()
    {
        $sections = array();
        while (($section = $this->get_section()) !== false)
        {
            $sections[] = $section;
        }
        return $sections;
    }

    public function norm_section($in)
    {
        $out = preg_replace("/[^\w ]/", "", $in);
        $out = trim($out);
        return strtolower($out);
    }

    public function get_section()
    {
        $this->eat_whitespace();
        $oldpos = ftell($this->fp);
        $sec = $this->get_line();
        if ($sec === false ||
            !in_array($this->norm_section($sec), $this->sections))
        {
            fseek($this->fp, $oldpos);
            return false;
        }

        return array(
            "name" => $sec,
            "entries" => $this->get_entries()
        );
    }

    public function get_entries()
    {
        $entries = array();
        $current = -1;
        $commonIndent = 8;
        $currentIndent = 0;

        while (!feof($this->fp))
        {
            $oldpos = ftell($this->fp);
            $line = $this->get_line();

            if (!empty($line) && (
                  ltrim($line) == $line || // end of release
                  in_array($this->norm_section($line), $this->sections) // end of section
               ))
            {
                fseek($this->fp, $oldpos);
                break;
            }

            $trimmed = trim($line);
            if (empty($trimmed) && $current == -1)
                continue;

            if (substr($trimmed, 0, 2) == "- ")
            {
                ++$current;
                $trimmed = substr($trimmed, 2);
                $currentIndent = $commonIndent + 2;
                $entries[$current] = $trimmed;
                continue;
            }

            preg_match("/^(\s+)/", $line, $matches);
            $newCurrentIndent = strlen($matches[1]);

            $padding = " ";
            if ($newCurrentIndent > $commonIndent &&
                $newCurrentIndent != $currentIndent)
            {
                $padding = "\n" . str_repeat("&nbsp;",
                                             $newCurrentIndent - $commonIndent - 2);
            }
            $currentIndent = $newCurrentIndent;

            $entries[$current] .= "$padding$trimmed";
        }

        return $entries;
    }

    public function eat_whitespace()
    {
        $off = ftell($this->fp);
        while (!feof($this->fp))
        {
            $ch = fgetc($this->fp);
            if (trim($ch) != '')
            {
                break;
            }
            $off++;
        }
        fseek($this->fp, $off);
    }

    public function get_line()
    {
        if (feof($this->fp))
            return false;
        return fgets($this->fp);
    }
}

header('Content-type: application/rss+xml');
echo '<?xml version="1.0" encoding="utf-8"?>';
$self = "http://{$_SERVER['SERVER_NAME']}{$_SERVER['SCRIPT_NAME']}";
?>
<rss version="2.0"
    xmlns:atom="http://www.w3.org/2005/Atom"
    xmlns:dc="http://purl.org/dc/elements/1.1/">
    <channel>
        <title>monotone - distributed version control</title>
        <atom:link href="<?php echo $self ?>" rel="self" type="application/rss+xml" />
        <link><?php echo $self ?></link>
        <description>Recent monotone releases</description>
        <language>en-us</language>

        <pubDate><?php echo date("r", $releases[0]['timestamp']); ?></pubDate>
        <?php foreach ($releases as $release): ?>
        <item>
            <title><?php echo $release['header']; ?></title>
            <description><![CDATA[

            <?php foreach ($release['sections'] as $section): ?>

                <h2><?php echo $section['name']; ?></h2>
                <ul>
                <?php foreach ($section['entries'] as $entry): ?>
                    <li><?php
                        // convert <, >, and additional spaces
                        $entry = str_replace('<', '&lt;', $entry);
                        $entry = preg_replace_callback('/([ ]{2,})/',
                           create_function('$matches', '
                               return str_repeat("&nbsp;", strlen($matches[1]));
                           '),
                           $entry
                        );

                        // link normal urls
                        $entry = preg_replace('#(https?://[^ )>\b]+)#',
                                              '<a href="$1">$1</a>',
                                              $entry);

                        // link old savannah bugs
                        $entry = preg_replace_callback(
                           '/monotone bugs?(?:(?:, |, and | and | )#\d+)+/',
                           create_function('$matches', '
                                return preg_replace(
                                   "/#(\d+)/",
                                   "<a href=\"https://savannah.nongnu.org/bugs/?$1\">#$1</a>",
                                   $matches[0]
                                );
                           '),
                           $entry);
                        // link new IDF issues (we use the same procedure as
                        // above, just that we call them now "issues" by convention
                        $entry = preg_replace_callback(
                           '/monotone issues?(?:(?:, |, and | and | )#?\d+)+/',
                           create_function('$matches', '
                                return preg_replace(
                                   "/#?(\d+)/",
                                   "<a href=\"http://code.mtnserv.thomaskeller.biz/index.php/p/monotone/issues/$1/\">#$1</a>",
                                   $matches[0]
                                );
                           '),
                           $entry);
                        echo nl2br($entry);
                    ?></li>
                <?php endforeach; ?>
                </ul>
            <?php endforeach; ?>
            ]]></description>

            <pubDate><?php echo date('r', $release['timestamp']); ?></pubDate>
        </item>
        <?php endforeach; ?>
    </channel>
</rss>

